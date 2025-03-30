#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int threadnum) 
        : threadnum_(threadnum), mainloop_(new EventLoop(true)), acceptor_(mainloop_.get(), ip, port),
        threadpool_(threadnum_, "IO")
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
sleep(1);
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

// 处理新客户端连接请求
void TcpServer::newconnection(std::unique_ptr<Socket> clientsock)  
{
    // 把新建的conn分配给从事件循环
    spConnection conn(new Connection(subloops_[clientsock->fd() % threadnum_].get(), std::move(clientsock)));
    conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2)); 
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));   
    
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->fd()] = conn;      // 把conn存放map容器中       
    }

    subloops_[conn->fd() % threadnum_]->newconnection(conn);    // 把conn存放到EventLoop的map容器中
    printf("TcpServer::newconnection() thread is %ld.\n",syscall(SYS_gettid)); 

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
    printf("TcpServer::removeconn() thread is %ld.\n",syscall(SYS_gettid)); 
    
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);   // 从map中删除conn
    }
}   