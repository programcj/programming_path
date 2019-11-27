/*
 * tcpdump.c
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
#include <ctype.h>
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
#include <sys/poll.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

typedef struct pcap_hdr_s
{
	uint32_t magic_number; /* magic number pcap文件标识 目前为“d4 c3 b2 a1”*/
	uint16_t version_major; /* major version number 2字节 主版本号    */
	uint16_t version_minor; /* minor version number  2字节 次版本号*/
	int32_t thiszone; /* GMT to local correction */
	uint32_t sigfigs; /* accuracy of timestamps */
	uint32_t snaplen; /* max length of captured packets, in octets */
	uint32_t network; /* data link type */
}__attribute__((packed)) pcap_hdr_t;
//magic：   4字节 pcap文件标识 目前为“d4 c3 b2 a1”
//major：   2字节 主版本号     #define PCAP_VERSION_MAJOR 2
//minor：   2字节 次版本号     #define PCAP_VERSION_MINOR 4
//thiszone：4字节 时区修正     并未使用，目前全为0
//sigfigs： 4字节 精确时间戳   并未使用，目前全为0
//snaplen： 4字节 抓包最大长度 如果要抓全，设为0x0000ffff（65535），
//         tcpdump -s 0就是设置这个参数，缺省为68字节
//linktype：4字节 链路类型    一般都是1：ethernet

typedef struct pcaprec_hdr_s
{
	uint32_t ts_sec; /* timestamp seconds */
	uint32_t ts_usec; /* timestamp microseconds */
	uint32_t incl_len; /*  */
	uint32_t orig_len; /* actual length of packet */
}__attribute__((packed)) pcaprec_hdr_t;

static int _sigint = 0;
void _sysquit(int sig)
{
	_sigint = 1;
}

int ToolNetDump(const char *ifrname)
{
	int fd = -1;
	struct ifreq ifr;
	int ret = -1;

	signal(SIGINT, _sysquit);
	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd == -1)
	{
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifrname);
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (ret)
	{
		printf("SIOCGIFINDEX ret=%d, err=%d, %s\n", ret, errno,
				strerror(errno));
		close(fd);
		return -1;
	}
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLL_IN;

	char buff[65535];
	int rlen = 0;
	char *ptr = NULL;

	struct timespec tp;
	FILE *fp = fopen("dump.pcap", "wb");
	pcap_hdr_t head;
	pcaprec_hdr_t item;

	memset(&head, 0, sizeof(head));
	memset(&item, 0, sizeof(item));

	head.magic_number = htonl(0xd4c3b2a1);
	head.version_major = 2;
	head.version_minor = 4;
	head.snaplen = sizeof(buff);
	head.network = 1;
	fwrite(&head, sizeof(pcap_hdr_t), 1, fp);
	int index = 0;

	while (!_sigint)
	{
		ret = poll(&fds, 1, 200);
		if (ret == 0)
			continue;
		if (ret < 0)
			break;
		rlen = recvfrom(fd, buff, sizeof(buff), 0, NULL, NULL);
		if (rlen <= 0)
		{
			perror("recvfrom():");
			break;
		}
		ptr = buff;
		struct ethhdr *pack_eth = (struct ethhdr *) ptr;
		ptr += sizeof(struct ethhdr);

		struct iphdr *pack_ip = (struct iphdr*) (ptr);
		ptr += sizeof(struct iphdr);

		struct udphdr *pack_udp = NULL;
		struct tcphdr *pack_tcp = NULL;

		if (pack_ip->protocol == IPPROTO_TCP)
		{
			pack_tcp = (struct tcphdr *) ptr;

			char sip[20];
			char dip[20];

			inet_ntop(AF_INET, &pack_ip->saddr, sip, sizeof(sip));
			inet_ntop(AF_INET, &pack_ip->daddr, dip, sizeof(dip));
			uint16_t sport = ntohs(pack_tcp->source);
			uint16_t dport = ntohs(pack_tcp->dest);
			if (sport == 50011 || dport == 50011)
			{
				clock_gettime(CLOCK_MONOTONIC, &tp);
				item.ts_sec = tp.tv_sec;
				item.ts_usec = tp.tv_nsec / 1000;
				item.incl_len = rlen;
				item.orig_len = rlen;
				fwrite(&item, sizeof(item), 1, fp);
				fwrite(buff, 1, rlen, fp);
				printf("index:%d, length:%d\n", index++, rlen);
			}
		}
		if (pack_ip->protocol == IPPROTO_UDP)
		{
			pack_udp = (struct udphdr *) ptr;
		}

	}
	fclose(fp);

	close(fd);

	return ret;
}

int socket_packet_tcp_recv(const char *interface)
{
	return 0;
}

//真正能够实现操作链路层数据的只有三种方式：
//int fd = socket (PF_INET, SOCK_PACKET, IPPROTO_TCP);
//int fd = socket (PF_PACKET, SOCK_RAW, IPPROTO_TCP);
//int fd = socket (PF_PACKET, SOCK_DGRAM, IPPROTO_TCP);
int socket_packet_udp_recv(const char *interface)
{
	int fd = -1;
	struct ifreq ifr;
	int ret = -1;

//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //struct ethhdr
//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
//fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP)); //从IP头开始抓
	if (fd < 0)
	{
		printf("ret=%d, err=%d, %s\n", fd, errno, strerror(errno));
	}
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, interface);
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);

	if (ret)
	{
		printf("SIOCGIFINDEX ret=%d, err=%d, %s\n", ret, errno,
				strerror(errno));
	}
//E0-D5-5E-90-D7-78
	uint8_t buff[65535]; //65535
	fd_set rset;
	int length;
	struct timeval tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	while (1)
	{
		select(fd + 1, &rset, NULL, NULL, &tv);

		memset(buff, 0, sizeof(buff));

		length = recvfrom(fd, buff, sizeof(buff), 0, NULL, NULL);
		if (length == -1)
		{
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

			inet_ntop(AF_INET, &pack_ip->saddr, sip, sizeof(sip));
			inet_ntop(AF_INET, &pack_ip->daddr, dip, sizeof(dip));

			if (pack_ip->protocol == IPPROTO_TCP)
			{
				struct tcphdr *pack_tcp = (struct tcphdr*) (buff + iplen);
				uint16_t sport = ntohs(pack_tcp->source);
				uint16_t dport = ntohs(pack_tcp->dest);

				//printf("tcp %s:%d->%s:%d\n", sip, sport, dip, dport);
			}
			if (pack_ip->protocol == IPPROTO_UDP)
			{
				pack_udp = (struct udphdr*) (buff + iplen);
				udp_data = buff + (iplen + sizeof(struct udphdr));
				udp_data_len = ntohs(pack_udp->len) - sizeof(struct udphdr);

				uint16_t sport = ntohs(pack_udp->source);
				uint16_t dport = ntohs(pack_udp->dest);

				printf("udp %s:%d->%s:%d, len:%d\n", sip, sport, dip, dport,
						udp_data_len);

				for (int i = 0; i < udp_data_len; i++)
				{
					printf("%02X ", udp_data[i]);
					if ((i + 1) % 16 == 0)
					{
						printf(" |");
						for (int j = i - 16; j < i; j++)
						{
							if (isgraph(udp_data[j]) && udp_data[j] != '\n'
									&& udp_data[j] != '\r')
								printf("%c", udp_data[j]);
							else
								printf(".");
						}
						printf("\n");
					}
				}
				printf("\n");
			}
		}
	}
	ret = 0;
	close(fd);
	return 0;
}
