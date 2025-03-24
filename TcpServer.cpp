#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port)
{
    Socket *servsock = new Socket(createnonblocking());
    InetAddress servaddr(ip, port); // 服务端的地址和协议。
    servsock->setreuseaddr(true);
    servsock->settcpnodelay(true);
    servsock->setreuseport(true);
    servsock->setkeepalive(true);
    servsock->bind(servaddr);
    servsock->listen();

    Channel *servchannel = new Channel(servsock->fd(), &loop_); // 这里new出来的对象没有释放，这个问题以后再解决
    servchannel->setreadcallback(std::bind(&Channel::newconnection, servchannel, servsock)); 
    servchannel->enablereading();       // 让epoll_wait()监视servchannel的读事件
}

TcpServer::~TcpServer()
{

}

void TcpServer::start()       // 运行事件循环
{
    loop_.run();
}