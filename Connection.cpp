#include "Connection.h"

Connection::Connection(EventLoop *loop, Socket *clientsock) : loop_(loop), clientsock_(clientsock)
{
    // 为新客户端连接准备读事件，并添加到epoll中
    Channel *clientchannel = new Channel(clientsock->fd(), loop_); 
    clientchannel->setreadcallback(std::bind(&Channel::onmessage, clientchannel));
    clientchannel->useet();                 // 客户端连上来的fd采用边缘触发
    clientchannel->enablereading();        // 让epoll_wait()监视clientchannel的读事件
}

Connection::~Connection()
{
    delete clientsock_;
    delete clientchannel_;
}