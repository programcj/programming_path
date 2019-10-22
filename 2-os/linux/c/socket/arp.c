/*
 * arp.c
 *
 *  Created on: 2019年10月22日
 *      Author: cc
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>

struct __attribute__ ((__packed__)) arp_pkt
{
	uint8_t dst_mac[6];
	uint8_t src_mac[6];
	uint16_t etype;
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t opcode;
	uint8_t sender_mac[6];
	struct in_addr sender_ip;
	uint8_t target_mac[6];
	struct in_addr target_ip;
};

int arp_build_packet(struct arp_pkt *pkt, const char *interface,
		const char *dscip) {
	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	int ret = 0;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", "eth0");
	ret = ioctl(fd, SIOCGIFHWADDR, &ifr);

	if (ret) {
		printf("ret=%d, err=%d, %s\n", ret, errno, strerror(errno));
	}

	memcpy(pkt->sender_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof(uint8_t));
	memcpy(pkt->src_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof(uint8_t));

	ret = ioctl(fd, SIOCGIFADDR, &ifr);
	if (ret) {
		printf("ret=%d, err=%d, %s\n", ret, errno, strerror(errno));
	}

	pkt->sender_ip = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr;
	pkt->htype = htons(1);
	pkt->ptype = htons(ETH_P_IP);
	pkt->hlen = 6;
	pkt->plen = 4;
	pkt->opcode = htons(ARPOP_REQUEST);

	memset(pkt->target_mac, 0, 6 * sizeof(uint8_t));
	memset(pkt->dst_mac, 0xFF, 6);

	pkt->etype = htons(ETH_P_ARP);

	pkt->target_ip.s_addr = inet_addr(dscip);
	if (fd > 0)
		close(fd);
	return 0;
}

int arp_send(struct arp_pkt *pkt, char *interface)
{
	int ret = -1;
	int sd;
	int bytes;
	struct sockaddr_ll device;
	memset(&device, 0, sizeof(device));
	device.sll_ifindex = if_nametoindex(interface);
	device.sll_family = AF_PACKET;
	device.sll_halen = htons(6);
	memcpy(device.sll_addr, pkt->sender_mac, 6 * sizeof(uint8_t));

	if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
		perror("socket() failed ");
		goto exit;
	}

	if ((bytes = sendto(sd, pkt, sizeof(struct arp_pkt), 0,
			(struct sockaddr *) &device, sizeof(device))) <= 0)
	{
		perror("sendto() failed");
		goto exit;
	}
	ret = 0;
	printf("arp send ok\n");
	exit: if (sd > 0)
		close(sd);
	return ret;
}

void arp_test()
{
	struct arp_pkt pkt;
	arp_build_packet(&pkt, "eth0", "192.168.200.1");
	for (int i = 0; i < 3; i++)
		arp_send(&pkt, "eth0");
}
