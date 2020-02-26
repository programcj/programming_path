#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/tcp.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

//获取远程主机的MAC地址,通过ARP缓存获取
//可参考:https://blog.csdn.net/qq_27627195/article/details/81942378
int getpeermac_by_ip(char *ipaddr, char* buf)
{
	int     sockfd;
	unsigned char *ptr;
	struct arpreq arpreq;
	struct sockaddr_in *sin;
	struct sockaddr_storage ss;
	char addr[INET_ADDRSTRLEN+1];

	memset(addr, 0, INET_ADDRSTRLEN+1);
	memset(&ss, 0, sizeof(ss));
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		perror("socket error");
		return -1;
	}
	sin = (struct sockaddr_in *) &ss;
	sin->sin_family = AF_INET;
	if (inet_pton(AF_INET, ipaddr, &(sin->sin_addr)) <= 0) {
		perror("inet_pton error");
		return -1;
	}
	sin = (struct sockaddr_in *) &arpreq.arp_pa;
	memcpy(sin, &ss, sizeof(struct sockaddr_in));
	strcpy(arpreq.arp_dev, "eth1");
	arpreq.arp_ha.sa_family = AF_UNSPEC;
	if (ioctl(sockfd, SIOCGARP, &arpreq) < 0) {
		perror("ioctl SIOCGARP: ");
		return -1;
	}
	ptr = (unsigned char *)arpreq.arp_ha.sa_data;
	sprintf(buf,"%x:%x:%x:%x:%x:%x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
	return 0;
}

int main(int argc, char const *argv[])
{
    char macstr[30];
    getpeermac_by_ip("192.168.0.230", macstr);
    printf("mac:%s\n", macstr);
    return 0;
}
