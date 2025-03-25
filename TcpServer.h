#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"

class TcpServer
{
private:
    EventLoop loop_;       // 一个TcpServer可以有多个事件循环，现在是单线程，暂时只用一个事件循环
    Acceptor *acceptor_;    // 一个TcpServer只有一个Acceptor对象

public:
    TcpServer(const std::string &ip, uint16_t port);
    ~TcpServer();

    void start();       // 运行事件循环
};