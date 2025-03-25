#include "Acceptor.h"

Acceptor::Acceptor(EventLoop *loop, const std::string &ip, uint16_t port) : loop_(loop)
{
    servsock_ = new Socket(createnonblocking());
    InetAddress servaddr(ip, port); // 服务端的地址和协议。
    servsock_->setreuseaddr(true);
    servsock_->settcpnodelay(true);
    servsock_->setreuseport(true);
    servsock_->setkeepalive(true);
    servsock_->bind(servaddr);
    servsock_->listen();

    Channel *acceptchannel_ = new Channel(servsock_->fd(), loop_); // 这里new出来的对象没有释放，这个问题以后再解决
    // acceptchannel_->setreadcallback(std::bind(&Channel::newconnection,acceptchannel_,servsock_));
    acceptchannel_->setreadcallback(std::bind(&Acceptor::newconnection, this)); 
    acceptchannel_->enablereading();       // 让epoll_wait()监视servchannel的读事件
}
Acceptor::~Acceptor()
{
    delete servsock_;
    // printf("delete servsock_ ok.\n");
    delete acceptchannel_;
}

#include "Connection.h"
// 处理新客户端连接请求
void Acceptor::newconnection()   
{
    InetAddress clientaddr; // 客户端的地址和协议。
    // 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
    // 还有，这里new出来的对象没有释放，这个问题以后再解决

    Socket *clientsock = new Socket(servsock_->accept(clientaddr));
    newconnectioncb_(clientsock);

}

// 设置处理新客户端连接请求的回调函数，将在创建Acceptor对象的时候（TcpServer类的构造函数中）设置
void Acceptor::setnewconnectioncb(std::function<void(Socket*)> fn)
{
    newconnectioncb_ = fn;
}
