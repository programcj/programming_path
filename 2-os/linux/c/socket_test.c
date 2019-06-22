#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char const *argv[])
{
    char ipstr[20];
    struct sockaddr_in saddr;
    saddr.sin_addr.s_addr=htonl(0x7b01a8c0); //0x7b01a8c0=123.1.168.192
    inet_ntop(AF_INET,&saddr.sin_addr, ipstr, sizeof(ipstr));
    printf("ip:%s\n", ipstr); //out 123.1.168.192

    inet_pton(AF_INET,"192.168.1.123", &saddr.sin_addr.s_addr); //s_addr 转换为网络字节序 7b01a8c0
    inet_ntop(AF_INET,&saddr.sin_addr, ipstr, sizeof(ipstr));
    printf("%x ip:%s\n", ntohl(saddr.sin_addr.s_addr), ipstr); //ntohl->out> 0xc0a8017b
    printf("0xC0=%d\n", 0xC0); //0xC0=192
    return 0;
}
