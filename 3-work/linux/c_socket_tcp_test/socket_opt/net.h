/*
 * net.h
 *
 *  Created on: 2019年4月6日
 *      Author: cj
 */

#ifndef C_SOCKET_TCP_TEST_SOCKET_OPT_NET_H_
#define C_SOCKET_TCP_TEST_SOCKET_OPT_NET_H_

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int ip_to_hostname(const char* ip) {
	int ret = 0;

	if (!ip) {
		printf("invalid params\n");
		return -1;
	}

	struct addrinfo hints;
	struct addrinfo *res, *res_p;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME | AI_NUMERICHOST;
	hints.ai_protocol = 0;

	ret = getaddrinfo(ip, NULL, &hints, &res);
	if (ret != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (res_p = res; res_p != NULL; res_p = res_p->ai_next) {
		char host[1024] = { 0 };
		ret = getnameinfo(res_p->ai_addr, res_p->ai_addrlen, host, sizeof(host),
		NULL, 0, NI_NAMEREQD);
		if (ret != 0) {
			printf("getnameinfo: %s\n", gai_strerror(ret));
		} else {
			printf("hostname: %s\n", host);
		}
	}

	freeaddrinfo(res);
	return ret;
}

int hostname_to_ip(const char* hostname) {
	int ret = 0;

	if (!hostname) {
		printf("invalid params\n");
		return -1;
	}

	struct addrinfo hints;
	struct addrinfo *res, *res_p;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = 0;

	ret = getaddrinfo(hostname, NULL, &hints, &res);
	if (ret != 0) {
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (res_p = res; res_p != NULL; res_p = res_p->ai_next) {
		char host[1024] = { 0 };
		ret = getnameinfo(res_p->ai_addr, res_p->ai_addrlen, host, sizeof(host),
		NULL, 0, NI_NUMERICHOST);
		if (ret != 0) {
			printf("getnameinfo: %s\n", gai_strerror(ret));
		} else {
			printf("ip: %s\n", host);
		}
	}

	freeaddrinfo(res);
	return ret;
}

#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <net/if_arp.h>

int show_mac(const char *ifname, const char *ipstr) {
	int sock_mac;

	struct ifreq ifr;
	char mac_addr[30];

	sock_mac = socket( AF_INET, SOCK_DGRAM, 0);
	if (sock_mac == -1) {
		perror("create socket falise...mac/n");
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	if ((ioctl(sock_mac, SIOCGIFHWADDR, &ifr)) < 0) {
		printf("mac ioctl error(%s) \n", strerror(errno));
		return -1;
	}
	sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned char) ifr.ifr_hwaddr.sa_data[0],
			(unsigned char) ifr.ifr_hwaddr.sa_data[1],
			(unsigned char) ifr.ifr_hwaddr.sa_data[2],
			(unsigned char) ifr.ifr_hwaddr.sa_data[3],
			(unsigned char) ifr.ifr_hwaddr.sa_data[4],
			(unsigned char) ifr.ifr_hwaddr.sa_data[5]);
	printf("%s ", mac_addr);

	{
		if (ioctl(sock_mac, SIOCGIFTXQLEN, &ifr) == 0) {
			printf("发送队列长度:%d ", ifr.ifr_qlen);
		}
	}
	{
		struct arpreq arpreq;
		struct sockaddr_in *sin;
		memset(&arpreq, 0, sizeof(arpreq));

		strcpy(arpreq.arp_dev, ifname);
		sin = (struct sockaddr_in *) &arpreq.arp_pa;
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = inet_addr("192.168.8.1");

		if (ioctl(sock_mac, SIOCGARP, &arpreq) == 0) {
			unsigned char *ptr = &arpreq.arp_ha.sa_data[0];
			printf(" %02x:%02x:%02x:%02x:%02x:%02x\n", *ptr, *(ptr + 1),
					*(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
		} else {
			printf(" NO ARPA[%s][%s][%s]", ifname, strerror(errno), ipstr);
		}
	}

	close(sock_mac);
	return 0;
}

int net_info() {
	struct sockaddr_in *sin = NULL;
	struct ifaddrs *ifa = NULL, *ifList;

	if (getifaddrs(&ifList) < 0) {
		return -1;
	}

	for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			printf("\n>>> interfaceName: %s\n", ifa->ifa_name);

			sin = (struct sockaddr_in *) ifa->ifa_addr;
			printf(" IP: %s ", inet_ntoa(sin->sin_addr));

			sin = (struct sockaddr_in *) ifa->ifa_dstaddr;
			printf("广播: %s ", inet_ntoa(sin->sin_addr));

			sin = (struct sockaddr_in *) ifa->ifa_netmask;
			printf("掩码: %s", inet_ntoa(sin->sin_addr));
			printf("\n mac:");
			show_mac(ifa->ifa_name,
					inet_ntoa(
							((struct sockaddr_in *) ifa->ifa_addr)->sin_addr));
		}
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin = (struct sockaddr_in6 *) ifa->ifa_addr;
			char str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &sin->sin6_addr, str, sizeof(str));
			//printf("IPV6:%s \n", str);
		}
	}
	freeifaddrs(ifList);
	return 0;
}

#define show_arp() system("cat /proc/net/arp")
#define show_route() system("cat /proc/net/route")
#include <netinet/ip_icmp.h>

enum {
	DEFDATALEN = 56,
	MAXIPLEN = 60,
	MAXICMPLEN = 76,
	MAX_DUP_CHK = (8 * 128),
	MAXWAIT = 10,
	PINGINTERVAL = 1, /* 1 second */
	pingsock = 0,
};
#define BB_LITTLE_ENDIAN 1
uint16_t inet_cksum(uint16_t *addr, int nleft) {
	/*
	 * Our algorithm is simple, using a 32 bit accumulator,
	 * we add sequential 16 bit words to it, and at the end, fold
	 * back all the carry bits from the top 16 bits into the lower
	 * 16 bits.
	 */
	unsigned sum = 0;
	while (nleft > 1) {
		sum += *addr++;
		nleft -= 2;
	}

	/* Mop up an odd byte, if necessary */
	if (nleft == 1) {
		if (BB_LITTLE_ENDIAN)
			sum += *(uint8_t*) addr;
		else
			sum += *(uint8_t*) addr << 8;
	}

	/* Add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16); /* add carry */

	return (uint16_t) ~sum;
}
#include <time.h>
#include <sys/time.h>
void net_ping(const char *dsc) {
	//AF_UNSPEC
	setuid(0);
	perror("");
	int sock = socket(AF_INET, SOCK_RAW, 1);
	if (sock < 0) {
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP); /* 1 == ICMP */
		if (sock < 0) {
			perror("socket err...");
			return;
		}
	}
	struct icmp *pkt;
	char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

	pkt = (struct icmp *) packet;
	/*memset(pkt, 0, sizeof(G.packet)); already is */
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0; /* icmp的编码 */
	pkt->icmp_cksum = 0; /* icmp的校验和 */
	pkt->icmp_seq = 1; /* icmp的顺序号 */
	pkt->icmp_id = 0; /* icmp的标志符 */
	//packsize = 8 + datalen;   /* icmp8字节的头 加上数据的长度(datalen=56), packsize = 64 */
	//tval = (struct timeval *)icmp->icmp_data;    /* 获得icmp结构中最后的数据部分的指针 */
	gettimeofday((struct timeval *)pkt->icmp_data, NULL); /* 将发送的时间填入icmp结构中最后的数据部分 */

	pkt->icmp_cksum = inet_cksum((uint16_t *) pkt, sizeof(packet));

	struct sockaddr_in *addr, sa;
//	struct addrinfo hints;
//	struct addrinfo *res, *cur;
//	hints.ai_family = AF_INET; /* Allow IPv4 */
//	hints.ai_flags = AI_PASSIVE;/* For wildcard IP address */
//	hints.ai_protocol = 0; /* Any protocol */
//	hints.ai_socktype = SOCK_STREAM;
//
//	char m_ipaddr[16];
//	struct sockaddr_in *addr, sa;
//	int ret = getaddrinfo(dsc, NULL, &hints, &res);
//	for (cur = res; cur != NULL; cur = cur->ai_next) {
//		addr = (struct sockaddr_in *) cur->ai_addr;
//		strcpy(m_ipaddr, inet_ntoa(addr->sin_addr));
//		printf("%s\n", m_ipaddr);
//	}
//	freeaddrinfo(res);
	int ret=0;
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = 0;
	sa.sin_addr.s_addr = inet_addr(dsc);
	int c;

	while (1) {
		ret = sendto(sock, packet, sizeof(packet), 0,
				(const struct sockaddr *) &sa, sizeof(sa));
		printf("star recv..\n");
		c = recv(sock, packet, sizeof(packet), 0);
		if (c < 0) {
			if (errno != EINTR)
				perror("recvfrom");
			break;
		}
		if (c >= 76) { /* ip + icmp */
			struct iphdr *iphdr = (struct iphdr *) packet;
			pkt = (struct icmp *) (packet + (iphdr->ihl << 2)); /* skip ip hdr */
			if (pkt->icmp_type == ICMP_ECHOREPLY) {
				printf("ICMP_ECHOREPLY\n");
				break;
			}
		}
	}
	close(sock);
}
#endif /* C_SOCKET_TCP_TEST_SOCKET_OPT_NET_H_ */
