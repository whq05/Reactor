#pragma once
#include <map>
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"

class TcpServer
{
private:
    EventLoop *mainloop_;       // 主事件循环
    std::vector<EventLoop*> subloops_;          // 存放从事件循环的容器                                        // 一个TcpServer可以有多个事件循环，现在是单线程，暂时只用一个事件循环
    Acceptor *acceptor_;                                           // 一个TcpServer只有一个Acceptor对象
    ThreadPool *threadpool_;                                     // 线程池
    int threadnum_;     // 线程池的大小，即从事件循环的个数
    std::map<int, Connection *> conns_;                            // 一个TcpServer有多个Connection对象，存放在map容器中
    std::function<void(Connection *)> newconnectioncb_;            // 回调EchoServer::HandleNewConnection()
    std::function<void(Connection *)> closeconnectioncb_;          // 回调EchoServer::HandleClose()
    std::function<void(Connection *)> errorconnectioncb_;          // 回调EchoServer::HandleError()
    std::function<void(Connection *, std::string &)> onmessagecb_; // 回调EchoServer::HandleMessage()
    std::function<void(Connection *)> sendcompletecb_;             // 回调EchoServer::HandleSendComplete()
    std::function<void(EventLoop *)> timeoutcb_;                   // 回调EchoServer::HandleEpollTimeout()

public:
    TcpServer(const std::string &ip, uint16_t port, int threadnum = 3);
    ~TcpServer();

    void start(); // 运行事件循环

    void newconnection(Socket *clientsock);                 // 处理新客户端连接请求，在Acceptor类中回调此函数
    void closeconnection(Connection *conn);                 // 关闭客户端的连接，在Connection类中回调此函数
    void errorconnection(Connection *conn);                 // 客户端的连接错误，在Connection类中回调此函数
    void onmessage(Connection *conn, std::string &message); // 处理客户端发送过来的消息，在Connection类中回调此函数
    void sendcomplete(Connection *conn);                    // 数据发送完成后，在Connection类中回调此函数
    void epolltimeout(EventLoop *loop);                     // epoll_wait()超时，在EventLoop类中回调此函数

    void setnewconnectioncb(std::function<void(Connection *)> fn);
    void setcloseconnectioncb(std::function<void(Connection *)> fn);
    void seterrorconnectioncb(std::function<void(Connection *)> fn);
    void setonmessagecb(std::function<void(Connection *, std::string &)> fn);
    void setsendcompletecb(std::function<void(Connection *)> fn);
    void settimeoutcb(std::function<void(EventLoop *)> fn);
};