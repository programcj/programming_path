/*
 * file_test.c
 *
 *  Created on: 2019年9月23日
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
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

int main(int argc, char **argv) {

	int sockfd = -1;
	struct ifreq ifr;
	struct sockaddr_in *addr = NULL;
	char ip_addr[50];

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, "eth0");
	addr = (struct sockaddr_in *) &ifr.ifr_addr;
	addr->sin_family = AF_INET;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket error!\n");
	}

	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
		strncpy(ip_addr, inet_ntoa(addr->sin_addr), sizeof(ip_addr));
		close(sockfd);
	}
	close(sockfd);
	printf("[%s]\n",ip_addr);
	//saddr.sin_addr.s_addr = inet_addr(ip_addr);
	return 0;
}

