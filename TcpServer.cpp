#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port)
{
    acceptor_ = new Acceptor(&loop_, ip, port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));
    loop_.setepolltimeoutcallback(std::bind(&TcpServer::epolltimeout, this, std::placeholders::_1));

}

TcpServer::~TcpServer()
{
    delete acceptor_;

    // 释放全部的Connection对象。
    for (auto &aa:conns_)
    {
        delete aa.second;
    } 
}

void TcpServer::start()       // 运行事件循环
{
    loop_.run();
}    

// 处理新客户端连接请求
void TcpServer::newconnection(Socket *clientsock)  
{
    Connection *conn = new Connection(&loop_, clientsock);
    conn->setclosecallback(std::bind(&TcpServer::closeconnection, this, std::placeholders::_1));
    conn->seterrorcallback(std::bind(&TcpServer::errorconnection, this, std::placeholders::_1));
    conn->setonmessagecallback(std::bind(&TcpServer::onmessage, this, std::placeholders::_1, std::placeholders::_2)); 
    conn->setsendcompletecallback(std::bind(&TcpServer::sendcomplete, this, std::placeholders::_1));       

    // printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", conn->fd(), conn->ip().c_str(), conn->port());

    conns_[conn->fd()] = conn;      // 把conn存放map容器中

    if (newconnectioncb_)   newconnectioncb_(conn);      // 回调EchoServer::HandleNewConnection()
}

void TcpServer::closeconnection(Connection *conn)     // 关闭客户端的连接，在Connection类中回调此函数
{
    if (closeconnectioncb_) closeconnectioncb_(conn);      // 回调EchoServer::HandleClose()
    // printf("client(eventfd=%d) disconnected.\n", conn->fd());
    conns_.erase(conn->fd());       // 从map中删除conn
    delete conn;
}

void TcpServer::errorconnection(Connection *conn)     // 客户端的连接错误，在Connection类中回调此函数
{
    if (errorconnectioncb_) errorconnectioncb_(conn);      // 回调EchoServer::HandleError()
    // printf("client(eventfd=%d) error.\n", conn->fd());
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
    // printf("send complete.\n");

    if (sendcompletecb_) sendcompletecb_(conn);      // 回调EchoServer::HandleSendComplete()
}

// epoll_wait()超时，在EventLoop类中回调此函数
void TcpServer::epolltimeout(EventLoop *loop)     
{
    // printf("epoll_wait() timeout.\n");

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
