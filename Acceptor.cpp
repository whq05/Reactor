#include "Acceptor.h"

Acceptor::Acceptor(const std::unique_ptr<EventLoop> &loop, const std::string &ip, uint16_t port) 
            : loop_(loop), servsock_(createnonblocking()), acceptchannel_(servsock_.fd(), loop_)
{
    // servsock_ = new Socket(createnonblocking());
    InetAddress servaddr(ip, port); // 服务端的地址和协议。
    servsock_.setreuseaddr(true);
    servsock_.settcpnodelay(true);
    servsock_.setreuseport(true);
    servsock_.setkeepalive(true);
    servsock_.bind(servaddr);
    servsock_.listen();

    // acceptchannel_ = new Channel(servsock_.fd(), loop_); 
    acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection, this)); 
    acceptchannel_.enablereading();       // 让epoll_wait()监视servchannel的读事件
}
Acceptor::~Acceptor()
{
    // delete servsock_;
    // delete acceptchannel_;
}

// 处理新客户端连接请求
void Acceptor::newconnection()   
{
    InetAddress clientaddr; // 客户端的地址和协议。

    std::unique_ptr<Socket> clientsock(new Socket(servsock_.accept(clientaddr)));
    clientsock->setipport(clientaddr.ip(), clientaddr.port());

    newconnectioncb_(std::move(clientsock));        // 回调TcpServer::newconnection()

}

// 设置处理新客户端连接请求的回调函数，将在创建Acceptor对象的时候（TcpServer类的构造函数中）设置
void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)
{
    newconnectioncb_ = fn;
}
