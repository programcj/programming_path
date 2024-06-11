/**********************************
 * tool_arp.c
 *
 * 发送arp请求.
 * Send arp request. need root
 **********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>

static int arp_pack_request(int sockfd, struct in_addr *src, struct in_addr *dsc,
		void *src_mac, void *dsc_mac, struct sockaddr_ll *addr) {
	unsigned char pack[200];

	struct arphdr *arph = (struct arphdr*) pack;
	unsigned char *p = pack + sizeof(struct arphdr);
	arph->ar_hrd = ntohs(ARPHRD_ETHER);
	arph->ar_pro = ntohs(ETH_P_IP);
	arph->ar_hln = 6;
	arph->ar_pln = 4;
	arph->ar_op = ntohs(ARPOP_REQUEST);

	p = (unsigned char *)mempcpy(p, src_mac, 6);
	p = mempcpy(p, src, 4);

	p = mempcpy(p, dsc_mac, 6);
	p = mempcpy(p, dsc, 4);
	p = mempcpy(p, "hello", 5);

	return sendto(sockfd, pack, p - pack, 0, (struct sockaddr *) addr,
			sizeof(*addr));
}

//只发送arg请求
//Who has <dsthost>? Tell <srcip>
int tool_arp_req(const char *ifrname, const char *srcip, const char *dsthost)
{
	int sock_fd;
	int ret;

	struct sockaddr_ll device;
	device.sll_family = AF_PACKET;
	device.sll_protocol = ntohs(ETH_P_ARP);

	struct ifreq ifr;
	struct in_addr src, dsc;

	inet_aton(srcip, &src);
	inet_aton(dsthost, &dsc);

	sock_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP));
	if (sock_fd < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifrname);
	ret = ioctl(sock_fd, SIOCGIFINDEX, &ifr);
	if (ret)
	{
		printf("SIOCGIFINDEX ret=%d, err=%d, %s\n", ret, errno,
					strerror(errno));
		close(sock_fd);
		return -1;
	}

	device.sll_ifindex = ifr.ifr_ifindex; //if_nametoindex(ifrname);
	strcpy(ifr.ifr_name, ifrname);

	if (ioctl(sock_fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		perror("ioctl() failed to get source MAC address ");
		close(sock_fd);
		return -1;
	}
	memcpy(device.sll_addr, ifr.ifr_hwaddr.sa_data, 6);
	device.sll_halen = htons(6);

	ret = bind(sock_fd, (struct sockaddr *) &device, sizeof(device)); //发送数据需要bind
	if (ret)
	{
		perror("binderr");
		close(sock_fd);
		return ret;
	}
	int recvsize = 1024;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &recvsize, sizeof(recvsize));

	struct sockaddr_ll lltar;
	lltar = device;
	memset(lltar.sll_addr, 0xFF, 8);

	uint8_t dsc_mac[6];
	memset(dsc_mac, 0, 6);
	ret = arp_pack_request(sock_fd, &src, &dsc, device.sll_addr, dsc_mac, &lltar);
	close(sock_fd);
	return ret;
}
