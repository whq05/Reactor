/*
echoserver.cpp，回显（EchoServer）服务器
*/

#include "EchoServer.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./echoserver ip port\n");
        printf("example: ./echoserver 192.168.206.132 5010\n\n");
        return -1;
    }
    
    // TcpServer tcpserver(argv[1], atoi(argv[2]));
    // tcpserver.start();      // 运行事件循环

    EchoServer echoserver(argv[1], atoi(argv[2]), 3, 3);
    echoserver.Start();      // 运行事件循环

    return 0;
}