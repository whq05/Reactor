// 网络通讯的客户端程序
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:./client ip port\n");
        printf("example:./client 192.168.206.132 5010\n\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("socket() failed.\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connect(%s:%s) failed.\n", argv[1], argv[2]);
        close(sockfd);
        return -1;
    }

    printf("connect ok.\n");
    // printf("开始时间：%d",time(0));

    /*
    // 对应sep_ = 1
    for (int ii = 0; ii < 100; ii++)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "you will get %d dollars", ii);

        char tmpbuf[1024];          // 临时的buffer，报文头部+报文内容
        memset(tmpbuf, 0, sizeof(tmpbuf));  
        int len = strlen(buf);           // 计算报文的大小
        memcpy(tmpbuf, &len, 4);  // 拼接报文头部
        memcpy(tmpbuf+4, buf, len);  // 拼接报文内容

        if (send(sockfd, tmpbuf, len + 4, 0) <= 0)      // 把请求报文发送给服务端
        {
            printf("write() failed.\n");
            close(sockfd);
            return -1;
        }

        recv(sockfd, &len, 4, 0);       // 先读取4字节的报文头部
        memset(buf, 0, sizeof(buf));
        if (recv(sockfd, buf, len, 0) <= 0) // 读取报文内容
        {
            printf("read() failed.\n");
            close(sockfd);
            return -1;
        }

        printf("recv:%s\n", buf);
// sleep(1);
    }
    */

    // 对应sep_ = 0
    for (int ii=0;ii<10;ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf, "you will get %d dollars", ii);

        send(sockfd,buf,strlen(buf),0);  // 把请求报文发送给服务端。
        
        memset(buf,0,sizeof(buf));
        recv(sockfd,buf,1024,0);           // 读取报文内容。

        printf("recv:%s\n",buf);
// sleep(1);
    }

   /*
    // 对应sep_ = 2
    for (int ii=0;ii<100;ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf, "you will get %d dollars\r\n\r\n", ii);

        send(sockfd,buf,strlen(buf),0);  // 把请求报文发送给服务端。
        
        memset(buf,0,sizeof(buf));
        
        recv(sockfd,buf,1024,0);           // 读取报文内容。
        buf[strlen(buf)-4]='\0';
        printf("recv:%s\n",buf);
// sleep(1);
    }
    */
    // printf("结束时间：%d",time(0));
}