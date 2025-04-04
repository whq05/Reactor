#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int threadnum, uint16_t sep) 
        : threadnum_(threadnum), mainloop_(new EventLoop(true)), acceptor_(mainloop_.get(), ip, port),
        threadpool_(threadnum_, "IO"), sep_(sep)
{
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));

    acceptor_.setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));

    // 创建从事件循环
    for (int ii = 0; ii < threadnum_; ii++)
    {
        subloops_.emplace_back(new EventLoop(false, 5, 10));         // 创建从事件循环，存入subloops_容器中
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));   // 设置timeout超时的回调函数
        subloops_[ii]->settimercallback(std::bind(&TcpServer::removeconn, this, std::placeholders::_1));    // 设置清理空闲TCP连接的回调函数
        threadpool_.addtask(std::bind(&EventLoop::run, subloops_[ii].get()));    // 在线程池中运行从事件循环
    }

    // 构建一致性哈希环（添加虚拟节点）
    for (int ii = 0; ii < threadnum_; ii++) 
    {
        EventLoop* subloop = subloops_[ii].get();
        // 每个EventLoop添加100个虚拟节点
        for (int j = 0; j < 100; j++) {
            std::string node_key = std::to_string(ii) + "_" + std::to_string(j);
            uint32_t hash = std::hash<std::string>{}(node_key);
            hash_ring_.emplace(hash, subloop);
        }
    }

}

TcpServer::~TcpServer()
{

}

// 运行事件循环
void TcpServer::start()       
{
    mainloop_->run();
}    

// 停止IO线程和事件循环
void TcpServer::stop()
{
    // 停止主事件循环
    mainloop_->stop();
    printf("主事件循环已停止。\n");

    // 停止从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_[ii]->stop();
    }
    printf("从事件循环已停止。\n");

    // 停止IO线程。
    threadpool_.stop();
    printf("IO线程池停止。\n");
}    

// 处理新客户端连接请求
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)  
{
    // 计算客户端标识的哈希值（这里用fd()，可根据需求改用IP+端口）
    uint32_t key_hash = std::hash<int>{}(clientsock->fd());

    // 查找最近的哈希节点
    auto it = hash_ring_.lower_bound(key_hash);
    if (it == hash_ring_.end()) 
    {  
        // 超出范围则选择第一个节点
        it = hash_ring_.begin();
    }
    EventLoop* target_loop = it->second;

    // 把新建的conn分配给从事件循环
    spConnection conn(new Connection(target_loop, std::move(clientsock), sep_));
    conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2)); 
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));   

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->fd()] = conn;      // 把conn存放map容器中       
    }

    subloops_[conn->fd() % threadnum_]->newconnection(conn);    // 把conn存放到EventLoop的map容器中

    if (newconnectioncb_)   newconnectioncb_(conn);      // 回调EchoServer::HandleNewConnection()
}

void TcpServer::closeconnection(spConnection conn)     // 关闭客户端的连接，在Connection类中回调此函数
{
    if (closeconnectioncb_) closeconnectioncb_(conn);      // 回调EchoServer::HandleClose()

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());       // 从map中删除conn
    }
}

void TcpServer::errorconnection(spConnection conn)     // 客户端的连接错误，在Connection类中回调此函数
{
    if (errorconnectioncb_) errorconnectioncb_(conn);      // 回调EchoServer::HandleError()
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->fd());       // 从map中删除conn
    }
}

void TcpServer::onmessage(spConnection conn, std::string &message)     // 处理客户端发送过来的消息，在Connection类中回调此函数
{
    if (onmessagecb_) onmessagecb_(conn, message);      // 回调EchoServer::HandleMessage()
}

// 数据发送完成后，在Connection类中回调此函数
void TcpServer::sendcomplete(spConnection conn)           
{
    if (sendcompletecb_) sendcompletecb_(conn);      // 回调EchoServer::HandleSendComplete()
}

// epoll_wait()超时，在EventLoop类中回调此函数
void TcpServer::epolltimeout(EventLoop *loop)     
{
    if (timeoutcb_) timeoutcb_(loop);      // 回调EchoServer::HandleEpollTimeout()
}

void TcpServer::setnewconnectioncb(std::function<void(spConnection)> fn)
{
    newconnectioncb_ = fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_ = fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_ = fn;
}

void TcpServer::setonmessagecb(std::function<void(spConnection, std::string &message)> fn)
{
    onmessagecb_ = fn;
}

void TcpServer::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_ = fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_ = fn;
}

// 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数
void TcpServer::removeconn(int fd)
{
    // printf("TcpServer::removeconn() thread is %ld.\n",syscall(SYS_gettid)); 
    
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);   // 从map中删除conn
    }
}   