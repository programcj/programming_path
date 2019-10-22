/*
 * socket_raw.c
 *
 *  Created on: 2019年10月22日
 *      Author: cc
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#undef __USE_MISC
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>

//真正能够实现操作链路层数据的只有三种方式：
//int fd = socket (PF_INET, SOCK_PACKET, IPPROTO_TCP);
//int fd = socket (PF_PACKET, SOCK_RAW, IPPROTO_TCP);
//int fd = socket (PF_PACKET, SOCK_DGRAM, IPPROTO_TCP);
int socket_packet_udp_recv(const char *interface) {
	int fd = -1;
	struct ifreq ifr;
	int ret = -1;

	//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //struct ethhdr
	//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	if (fd < 0) {
		printf("ret=%d, err=%d, %s\n", fd, errno, strerror(errno));
	}
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, interface);
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (ret) {
		printf("ret=%d, err=%d, %s\n", ret, errno, strerror(errno));
	}
	//E0-D5-5E-90-D7-78
	uint8_t buff[4096];
	char lxjsonStr[4096];
	fd_set rset;
	int length;

	int i;
	struct timeval tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	while (1) {
		select(fd + 1, &rset, NULL, NULL, &tv);

		memset(buff, 0, sizeof(buff));

		length = recvfrom(fd, buff, sizeof(buff), 0, NULL, NULL);
		if (length == -1) {
			perror("recvfrom():");
			break;
		}
		//print_hex(buff, length);
		{
			struct iphdr *pack_ip = (struct iphdr*) buff;
			struct udphdr *pack_udp = NULL;
			uint8_t *udp_data = NULL;
			int udp_data_len = 0;
			int iplen = (pack_ip->ihl & 0x0f) * 4;
			char sip[20];
			char dip[20];

			if (pack_ip->protocol == IPPROTO_UDP) {
				inet_ntop(AF_INET, &pack_ip->saddr, sip, sizeof(sip));
				inet_ntop(AF_INET, &pack_ip->daddr, dip, sizeof(dip));

				pack_udp = (struct udphdr*) (buff + iplen);
				udp_data = buff + (iplen + sizeof(struct udphdr));
				udp_data_len = ntohs(pack_udp->len) - sizeof(struct udphdr);

				uint16_t sport = ntohs(pack_udp->source);
				uint16_t dport = ntohs(pack_udp->dest);

				printf("udp %s:%d->%s:%d, len:%d\n", sip, sport, dip, dport,
						udp_data_len);
			}
		}
	}
	ret = 0;
	close(fd);
	return 0;
}

int socket_inet_udp_recv() {
	int fd = -1;
	int rlen = 0;
	uint8_t buff[4096];
	struct ifreq ifr;
	int ret;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (fd < 0) {
		printf("fd=%d, err=%d, %s\n", fd, errno, strerror(errno));
	}
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth0");
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (ret) {
		printf("ret=%d, err=%d, %s\n", ret, errno, strerror(errno));
	}

	while (1) {
		memset(buff,0,sizeof(buff));
		rlen = recv(fd, buff, sizeof(buff), 0);
		if (rlen > 0) {
			{
				struct iphdr *pack_ip = (struct iphdr*) buff;
				struct udphdr *pack_udp = NULL;
				uint8_t *udp_data = NULL;
				int udp_data_len = 0;
				int iplen = (pack_ip->ihl & 0x0f) * 4;
				char sip[20];
				char dip[20];

				if (pack_ip->protocol == IPPROTO_UDP) {
					inet_ntop(AF_INET, &pack_ip->saddr, sip, sizeof(sip));
					inet_ntop(AF_INET, &pack_ip->daddr, dip, sizeof(dip));

					pack_udp = (struct udphdr*) (buff + iplen);
					udp_data = buff + (iplen + sizeof(struct udphdr));
					udp_data_len = ntohs(pack_udp->len) - sizeof(struct udphdr);

					uint16_t sport = ntohs(pack_udp->source);
					uint16_t dport = ntohs(pack_udp->dest);

					printf("udp %s:%d->%s:%d, len:%d, %s\n", sip, sport, dip, dport,
							udp_data_len, udp_data);
				}
			}
		}
	}
	close(fd);
	return 0;
}
