#include "Channel.h"

Channel::Channel(int fd, Epoll *ep) : fd_(fd), ep_(ep)      // 构造函数
{

}

Channel::~Channel()             // 析构函数
{
    // 在析构函数中，不要销毁ep_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已
}

int Channel::fd()         // 返回fd_成员
{
    return fd_;
}

void Channel::useet()                   // 采用边缘触发    
{
    events_ |= EPOLLET;
} 

void Channel::enablereading()           // 让epoll监视fd_的读事件
{
    events_ |= EPOLLIN;
    ep_->updatechannel(this);
}
void Channel::setinepoll()              // 把inepoll_成员的值设置为true
{
    inepoll_ = true;
}
void Channel::setrevents(uint32_t ev)   // 设置revents_成员的值为参数ev
{
    revents_ = ev;
}
bool Channel::inepoll()                 // 返回inepoll_成员
{
    return inepoll_;
}
uint32_t Channel::events()              // 返回events_成员
{
    return events_;
}

uint32_t Channel::revents()             // 返回revents_成员
{
    return revents_;
}