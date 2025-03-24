#include "EventLoop.h"

// 在构造函数中创建Epoll对象ep_
EventLoop::EventLoop() : ep_(new Epoll())     
{

}

 // 在析构函数中销毁ep_
EventLoop::~EventLoop()      
{
    delete ep_;
}

void EventLoop::run()         // 运行事件循环
{
    while (true) // 事件循环
    {
        std::vector<Channel *> channels = ep_->loop(); // 存放epoll_wait()返回事件
        for (auto &ch : channels)
        {
            ch->handleevent(); // 处理epoll_wait()返回的事件
        }
    }
}

// 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件
void EventLoop::updatechannel(Channel *ch)
{
    ep_->updatechannel(ch);
}