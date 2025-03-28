#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int threadnum) : threadnum_(threadnum)
{
    mainloop_ = new EventLoop;      // 创建主事件循环
    mainloop_->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));

    acceptor_ = new Acceptor(mainloop_, ip, port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));

    threadpool_ = new ThreadPool(threadnum_);    // 创建线程池

    // 创建从事件循环
    for (int ii = 0; ii < threadnum_; ii++)
    {
        subloops_.push_back(new EventLoop);         // 创建从事件循环，存入subloops_容器中
        subloops_[ii]->setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));
        threadpool_->addtask(std::bind(&EventLoop::run, subloops_[ii]));    // 在线程池中运行从事件循环
    }
}

TcpServer::~TcpServer()
{
    delete mainloop_;
    delete acceptor_;

    // 释放全部的Connection对象。
    for (auto &aa:conns_)
    {
        delete aa.second;
    } 

    // 释放从事件循环
    for (auto &aa : subloops_)
        delete aa;

    delete threadpool_;     // 释放线程池
}

// 运行事件循环
void TcpServer::start()       
{
    mainloop_->run();
}    

// 处理新客户端连接请求
void TcpServer::newconnection(Socket *clientsock)  
{
    // Connection *conn = new Connection(mainloop_, clientsock);
    // 把新建的conn分配给从事件循环
    Connection *conn = new Connection(subloops_[clientsock->fd() % threadnum_], clientsock);
    conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2)); 
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));       

    conns_[conn->fd()] = conn;      // 把conn存放map容器中

    if (newconnectioncb_)   newconnectioncb_(conn);      // 回调EchoServer::HandleNewConnection()
}

void TcpServer::closeconnection(Connection *conn)     // 关闭客户端的连接，在Connection类中回调此函数
{
    if (closeconnectioncb_) closeconnectioncb_(conn);      // 回调EchoServer::HandleClose()
    conns_.erase(conn->fd());       // 从map中删除conn
    delete conn;
}

void TcpServer::errorconnection(Connection *conn)     // 客户端的连接错误，在Connection类中回调此函数
{
    if (errorconnectioncb_) errorconnectioncb_(conn);      // 回调EchoServer::HandleError()
    conns_.erase(conn->fd());       // 从map中删除conn
    delete conn;
}

void TcpServer::onmessage(Connection *conn, std::string &message)     // 处理客户端发送过来的消息，在Connection类中回调此函数
{
    if (onmessagecb_) onmessagecb_(conn, message);      // 回调EchoServer::HandleMessage()
}

// 数据发送完成后，在Connection类中回调此函数
void TcpServer::sendcomplete(Connection *conn)           
{
    if (sendcompletecb_) sendcompletecb_(conn);      // 回调EchoServer::HandleSendComplete()
}

// epoll_wait()超时，在EventLoop类中回调此函数
void TcpServer::epolltimeout(EventLoop *loop)     
{
    if (timeoutcb_) timeoutcb_(loop);      // 回调EchoServer::HandleEpollTimeout()
}

void TcpServer::setnewconnectioncb(std::function<void(Connection*)> fn)
{
    newconnectioncb_ = fn;
}

void TcpServer::setcloseconnectioncb(std::function<void(Connection*)> fn)
{
    closeconnectioncb_ = fn;
}

void TcpServer::seterrorconnectioncb(std::function<void(Connection*)> fn)
{
    errorconnectioncb_ = fn;
}

void TcpServer::setonmessagecb(std::function<void(Connection*, std::string &)> fn)
{
    onmessagecb_ = fn;
}

void TcpServer::setsendcompletecb(std::function<void(Connection*)> fn)
{
    sendcompletecb_ = fn;
}

void TcpServer::settimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_ = fn;
}
