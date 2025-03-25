#pragma once
#include <functional>
#include "EventLoop.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"

class Acceptor
{
private:
    EventLoop *loop_;       // Acceptor对应的事件循环，在构造函数中传入
    Socket *servsock_;      // 服务端用于监听的socket，在构造函数中创建
    Channel *servchannel_;  // 服务端用于监听的channel，在构造函数中创建

public:
    Acceptor(EventLoop *loop, const std::string &ip, uint16_t port);
    ~Acceptor();

};