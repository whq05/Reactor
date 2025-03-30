#include "Connection.h"

Connection::Connection(EventLoop *loop, std::unique_ptr<Socket> clientsock)
    : loop_(loop), clientsock_(std::move(clientsock)), disconnect_(false), clientchannel_(new Channel(clientsock_->fd(), loop_))
{
    // 为新客户端连接准备读事件，并添加到epoll中
    // clientchannel_ = new Channel(clientsock_->fd(), loop_);
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage, this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback, this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback, this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback, this));
    clientchannel_->useet();         // 客户端连上来的fd采用边缘触发
    clientchannel_->enablereading(); // 让epoll_wait()监视clientchannel的读事件
}

Connection::~Connection()
{
    printf("conn已析构。\n");
}

int Connection::fd() const // 返回客户端的fd
{
    return clientsock_->fd();
}

std::string Connection::ip() const // 返回客户端的ip
{
    return clientsock_->ip();
}

uint16_t Connection::port() const // 返回客户端的port
{
    return clientsock_->port();
}

void Connection::closecallback() // TCP连接关闭（断开）的回调函数，供Channel回调
{
    disconnect_ = true;
    clientchannel_->remove();           // 从事件循环中删除Channel
    closecallback_(shared_from_this()); // 回调TcpServer::closeconnection()
}
void Connection::errorcallback() // TCP连接错误的回调函数，供Channel回调
{
    disconnect_ = true;
    clientchannel_->remove();           // 从事件循环中删除Channel
    errorcallback_(shared_from_this()); // 回调TcpServer::errorconnection()
}

void Connection::setclosecallback(std::function<void(spConnection)> fn) // 设置关闭fd_的回调函数
{
    closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(spConnection)> fn) // 设置fd_发生了错误的回调函数
{
    errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(spConnection, std::string &)> fn) // 设置处理报文的回调函数
{
    onmessagecallback_ = fn;
}

void Connection::setsendcompletecallback(std::function<void(spConnection)> fn) // 发送数据完成后的回调函数
{
    sendcompletecallback_ = fn;
}

// 处理对端发送过来的消息
void Connection::onmessage()
{
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
        if (nread > 0) // 成功的读取到了数据
        {
            inputbuffer_.append(buffer, nread); // 把读取的数据追加到接收缓冲区中
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取
        {
            continue;
        }
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕
        {
            while (true)
            {
                //////////////////////////////////////////////////////////////
                // 可以把以下代码封装在Buffer类中，还可以支持固定长度、指定报文长度和分隔符等多种格式
                int len;
                memcpy(&len, inputbuffer_.data(), 4); // 从inputbuffer中获取报文头部
                // 如果inputbuffer中的数据量小于报文头部，说明inputbuffer中的报文内容不完整
                if (inputbuffer_.size() < len + 4)
                    break;

                std::string message(inputbuffer_.data() + 4, len); // 从inputbuffer中获取一个报文
                inputbuffer_.erase(0, len + 4);                    // 从inputbuffer中删除刚才已获取的报文
                //////////////////////////////////////////////////////////////

                printf("message (eventfd=%d):%s\n", fd(), message.c_str());
                // updatelastatime_();  
                lastatime_ = Timestamp::now();
                std::cout << "lastatime=" << lastatime_.tostring() << std::endl;
                onmessagecallback_(shared_from_this(), message); // 回调TcpServer::onmessage()处理客户端的请求消息
            }
            break;
        }
        else if (nread == 0) // 客户端连接已断开
        {
            closecallback();
            break;
        }
    }
}

/*
这是使用深拷贝来解决客户端接收到的消息乱码

// 发送数据，不管在任何线程中，都是调用此函数发送数据
void Connection::send(const char *data, size_t size)
{
    if (disconnect_==true)
    {
        printf("客户端连接已断开了，send()直接返回。\n");
        return;
    }

    std::string message(data, size);
    if (loop_->isinloopthread())    // 判断当前线程是否为事件循环线程（IO线程）
    {
        // 如果当前线程是IO线程，直接调用sendinloop()发送数据
        printf("send() 在事件循环的线程中。\n");
        sendinloop(message);
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程去执行
        printf("send() 不在事件循环的线程中。\n");
        printf("Connection::send message1 = %s\n", data);
        // loop_->queueinloop(std::bind(&Connection::sendinloop, this, data, size));
        loop_->queueinloop(std::bind(&Connection::sendinloop, this, message));
        printf("Connection::send message2 = %s\n", data);
    }

}

// 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行
void Connection::sendinloop(const std::string &message)
{
    printf("Connection::sendinloop message = %s\n", message.c_str());
    outputbuffer_.appendwithhead(message.data(), message.size());   // 把需要发送的数据保存到Connection的发送缓冲区中
    clientchannel_->enablewriting();    // 注册写事件
}
*/

// 这是使用移动语义来解决客户端接收到的消息乱码
void Connection::send(std::string &&message)
{
    if (disconnect_)
    {
        printf("客户端连接已断开，send() 直接返回。\n");
        return;
    }

    // 打印调试信息（此时 message 尚未被移动）
    // printf("Connection::send message1 = %s\n", message.c_str());

    if (loop_->isinloopthread())
    {
        // 当前是 IO 线程，直接移动 message
        printf("send() 在事件循环的线程中。\n");
        sendinloop(std::move(message));
    }
    else
    {
        // 跨线程传递，将 message 移动到 bind 对象中
        printf("send() 不在事件循环的线程中。\n");
        loop_->queueinloop(std::bind(&Connection::sendinloop, this, std::move(message)));
    }

    // 注意：此处不能再访问 message（已被移动）
}

void Connection::sendinloop(std::string message)
{
    // 此处 message 是通过移动构造获取的数据，无拷贝
    // printf("Connection::sendinloop message = %s\n", message.c_str());
    outputbuffer_.appendwithhead(message.data(), message.size());
    clientchannel_->enablewriting();
}

void Connection::writecallback() // 处理写事件的回调函数，供Channel回调
{
    int writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0); // 尝试把outputbuffer_中的数据全部发送出去
    if (writen > 0)
        outputbuffer_.erase(0, writen); // 把已经发送出去的数据从outputbuffer_中删除

    // 如果发送缓冲区中没有数据了，表示数据已发送完成，不再关注写事件
    if (outputbuffer_.size() == 0)
    {
        clientchannel_->disablewriting();
        sendcompletecallback_(shared_from_this());
    }
}

// 更新Connection的时间戳
// void Connection::updatelastatime_()  
// {
//     lastatime_ = Timestamp::now(); 
// }

// 判断TCP连接是否超时（空闲太久）
bool Connection::timeout(time_t now, int val)  
{
    // std::cout << "lastatime=" << lastatime_.tostring() << std::endl;
    return now - lastatime_.toint() > val;
}