#include "InetAddress.h"

InetAddress::InetAddress()
{
}

InetAddress::InetAddress(const std::string &ip, uint16_t port) // 如果是监听的fd，用这个构造函数
{
    addr_.sin_family = AF_INET;                    // IPv4网络协议的套接字类型
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 服务端用于监听的ip地址
    addr_.sin_port = htons(port);                  // 服务端用于监听的端口
}

InetAddress::InetAddress(const sockaddr_in addr) : addr_(addr) // 如果是客户端连上来的fd，用这个构造函数
{
}

InetAddress::~InetAddress()
{
}

const char *InetAddress::ip() const // 返回字符串表示的地址，例如：192.168.206.132
{
    return inet_ntoa(addr_.sin_addr);
}

uint16_t InetAddress::port() const // 返回整数表示的端口，例如：80、8080
{
    return ntohs(addr_.sin_port);
}

const sockaddr *InetAddress::addr() const // 返回addr_成员的地址，转换成了sockaddr
{
    return (sockaddr *)&addr_;
}

void InetAddress::setaddr(sockaddr_in clientaddr) // 设置addr_成员的值。
{
    addr_ = clientaddr;
}