#pragma once
#include <functional>
#include "Epoll.h"

class Channel;
class Epoll;

class EventLoop
{
private:
    Epoll *ep_;      // 每个事件循环只有一个Epoll
    std::function<void(EventLoop*)> epolltimeoutcallback_;     // epoll_wait()超时的回调函数，将回调TcpServer::epolltimeout()
public:
    EventLoop();        // 在构造函数中创建Epoll对象ep_
    ~EventLoop();       // 在析构函数中销毁ep_

    void run();         // 运行事件循环

    void updatechannel(Channel *ch); 
    void setepolltimeoutcallback(std::function<void(EventLoop*)> fn);    // 设置epoll_wait()超时的回调函数

};