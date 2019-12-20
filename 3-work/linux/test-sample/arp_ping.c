/*
 * arp_ping.c
 *
 *  Created on: 2019年11月28日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
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

struct __attribute__((packed)) arpipv4 {
	unsigned char srcmac[6];
	struct in_addr srcaddr;
	unsigned char dscmac[6];
	struct in_addr dscaddr;
};

int arp_pack_send(int sockfd, struct in_addr *src, struct in_addr *dsc,
		void *src_mac, void *dsc_mac, struct sockaddr_ll *addr) {
	unsigned char pack[200];

	struct arphdr *arph = (struct arphdr*) pack;
	unsigned char *p = pack + sizeof(struct arphdr);
	arph->ar_hrd = ntohs(ARPHRD_ETHER);
	arph->ar_pro = ntohs(ETH_P_IP);
	arph->ar_hln = 6;
	arph->ar_pln = 4;
	arph->ar_op = ntohs(ARPOP_REQUEST);

	p = mempcpy(p, src_mac, 6);
	p = mempcpy(p, src, 4);

	p = mempcpy(p, dsc_mac, 6);
	p = mempcpy(p, dsc, 4);
	p = mempcpy(p, "hello", 5);

	return sendto(sockfd, pack, p - pack, 0, (struct sockaddr *) addr,
			sizeof(*addr));
}

int arp_ping(const char *ifrname, const char *host, unsigned char hostmac[6]) {
	//int sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	int sock_fd = socket(AF_PACKET, SOCK_DGRAM, 0);
	int ret;

	struct sockaddr_ll device;
	struct in_addr src, dsc;
	int exists = 0;
	memset(&dsc, 0, sizeof(dsc));

	if (!ifrname)
		ifrname = "eth0";
	{
		struct ifreq ifr;
		memset(&ifr, 0, sizeof(ifr));

		strcpy(ifr.ifr_name, ifrname);
		ret = ioctl(sock_fd, SIOCGIFINDEX, &ifr);
		if (ret) {
			printf("SIOCGIFINDEX ret=%d, err=%d, %s\n", ret, errno,
					strerror(errno));
			close(sock_fd);
			return -1;
		}
		device.sll_ifindex = ifr.ifr_ifindex; //if_nametoindex(ifrname);
		strcpy(ifr.ifr_name, ifrname);
		if (ioctl(sock_fd, SIOCGIFHWADDR, &ifr) < 0) {
			perror("ioctl() failed to get source MAC address ");
			return -1;
		}
		memcpy(device.sll_addr, ifr.ifr_hwaddr.sa_data, 6);
		device.sll_halen = htons(6);
		if (ioctl(sock_fd, SIOCGIFADDR, &ifr) < 0) {
			perror("ioctl() failed to get source MAC address ");
			return -1;
		}
		memcpy(&src, &((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr,
				sizeof(src));
	}

	device.sll_family = AF_PACKET;
	device.sll_protocol = ntohs(ETH_P_ARP);

	ret = bind(sock_fd, (struct sockaddr *) &device, sizeof(device)); //发送数据需要bind
	if (ret) {
		perror("binderr");
		close(sock_fd);
		return ret;
	}

	{
		int size = 1024;
		setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	}

	int rlen;
	unsigned char buff[300];
	struct pollfd fds;
	fds.fd = sock_fd;
	fds.events = POLL_IN;

	uint8_t dsc_mac[6];
	memset(dsc_mac, 0, 6);
	struct sockaddr_ll lltar;
	lltar = device;

	inet_aton(host, &dsc);

	time_t oldtm = 0;
	int sendcount = 0;
	memset(lltar.sll_addr, 0xFF, 8);
	while (sendcount < 3) {
		if (time(NULL) - oldtm > 1) {
			ret = arp_pack_send(sock_fd, &src, &dsc, device.sll_addr, dsc_mac,
					&lltar);
			//printf("arp send ret=%d\n", ret);
			if (ret <= 0) {
				perror("not send arp req");
				break;
			}
			oldtm = time(NULL);
			sendcount++;
		}

		ret = poll(&fds, 1, 200);
		if (ret == 0)
			continue;
		if (ret < 0) {
			perror("err...");
			break;
		}
		rlen = recvfrom(sock_fd, buff, sizeof(buff), 0, NULL, NULL);
		{
			struct arphdr *arph = (struct arphdr*) buff;
			struct arpipv4 *p = (struct arpipv4 *) (buff + sizeof(*arph));
			if (ARPOP_REQUEST == ntohs(arph->ar_op)) {
				//printf("arp req...\n");
				continue;
			}
			if (ARPOP_REPLY == ntohs(arph->ar_op)) {
				if (p->dscaddr.s_addr == src.s_addr) {
					exists = 1;
					printf("arp reply:");
					printf("src:%s ", inet_ntoa(p->srcaddr));
					printf("dsc:%s", inet_ntoa(p->dscaddr));
					printf(" MAC:%lX", *(uint64_t*) p->srcmac);
					if (hostmac) {
						memcpy(hostmac, p->srcmac, 6);
					}
					printf("\n");
					break;
				}
			}
		}
	}
	if (exists)
		ret = 0;
	else
		ret = 1;
	close(sock_fd);
	return ret;
}
