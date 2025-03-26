#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port)
{
    acceptor_ = new Acceptor(&loop_, ip, port);
    acceptor_->setnewconnectioncb(std::bind(&TcpServer::newconnection, this, std::placeholders::_1));
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

    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", conn->fd(), conn->ip().c_str(), conn->port());

    conns_[conn->fd()] = conn;      // 把conn存放map容器中
}

void TcpServer::closeconnection(Connection *conn)     // 关闭客户端的连接，在Connection类中回调此函数
{
    printf("client(eventfd=%d) disconnected.\n", conn->fd());
    // close(fd());        // 关闭客户端的fd
    conns_.erase(conn->fd());       // 从map中删除conn
    delete conn;
}

void TcpServer::errorconnection(Connection *conn)     // 客户端的连接错误，在Connection类中回调此函数
{
    printf("client(eventfd=%d) error.\n", conn->fd());
    // close(fd());            // 关闭客户端的fd
    conns_.erase(conn->fd());       // 从map中删除conn
    delete conn;
}

void TcpServer::onmessage(Connection *conn, std::string message)     // 处理客户端发送过来的消息，在Connection类中回调此函数
{
    // 在这里，将经过若干步骤的运算
    message = "reply:" + message; 

    int len = message.size();                   // 计算回应报文的大小
    std::string tmpbuf((char*)&len, 4);     // 把报文头部填充到回应报文中
    tmpbuf.append(message);                 // 把报文内容填充到回应报文中

    conn->send(tmpbuf.data(), tmpbuf.size());       // 把临时缓冲区中的数据发送出去
    // printf("TcpServer::onmessage\n");
}