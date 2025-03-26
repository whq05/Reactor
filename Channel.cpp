#include "Channel.h"

// 构造函数
Channel::Channel(int fd, EventLoop *loop) : fd_(fd), loop_(loop)
{
}

Channel::~Channel() // 析构函数
{
    // 在析构函数中，不要销毁ep_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已
}

int Channel::fd() // 返回fd_成员
{
    return fd_;
}

void Channel::useet() // 采用边缘触发
{
    events_ |= EPOLLET;
}

void Channel::enablereading() // 让epoll监视fd_的读事件
{
    events_ |= EPOLLIN;
    loop_->updatechannel(this);
}
void Channel::setinepoll() // 把inepoll_成员的值设置为true
{
    inepoll_ = true;
}
void Channel::setrevents(uint32_t ev) // 设置revents_成员的值为参数ev
{
    revents_ = ev;
}
bool Channel::inepoll() // 返回inepoll_成员
{
    return inepoll_;
}
uint32_t Channel::events() // 返回events_成员
{
    return events_;
}

uint32_t Channel::revents() // 返回revents_成员
{
    return revents_;
}

// 事件处理函数，epoll_wait()返回的时候，执行它
void Channel::handleevent()
{
    if (revents_ & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0
    {
        // printf("client(eventfd=%d) disconnected.\n", fd_);
        // close(fd_); // 关闭客户端的fd
        closecallback_();   // 回调Connection::closecallback()
    }
    else if (revents_ & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读
    {
        readcallback_();
    }
    else if (events_ & EPOLLOUT) // 有数据需要写，暂时没有代码
    {
    }
    else // 其它事件，都视为错误
    {
        // printf("client(eventfd=%d) error.\n", fd_);
        // close(fd_); // 关闭客户端的fd
        errorcallback_();
    }
}

void Channel::setreadcallback(std::function<void()> fn) // 设置fd_读事件的回调函数
{
    readcallback_ = fn;
}

void Channel::setclosecallback(std::function<void()> fn)    // 设置关闭fd_的回调函数
{
    closecallback_ = fn;
}

void Channel::seterrorcallback(std::function<void()> fn)    // 设置fd_发生了错误的回调函数
{
    errorcallback_ = fn;
}
