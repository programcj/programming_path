/*
 ============================================================================
 Name        : poll socket client test
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int socket_connect(const char *ip, uint16_t port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	if (sock == -1)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	int ret = connect(sock, (struct sockaddr*) &addr, sizeof(addr));
	if (ret == -1) {
		close(sock);
		return -1;
	}
	return sock;
}

int main(void) {
	struct pollfd pds;
	int sock = socket_connect("192.168.0.189", 22);
	int ret = 0;
	int opt = 0;

	if (sock == -1) {
		return 0;
	}

	opt = fcntl(sock, F_GETFL, 0);
	opt |= O_NONBLOCK;
	ret = fcntl(sock, F_SETFL, opt);

	printf("start poll %d \n", sock);

	int i = 0;
	sleep(1);
	while (1) {
		memset(&pds, 0, sizeof(pds));
		pds.fd = sock;
		pds.events = POLLOUT | POLLIN | POLLERR | POLLHUP | POLLNVAL;

		i++;

		ret = poll(&pds, 1, 200);

		if (ret < 0) {
			perror("poll");
			break;
		}
		if (ret == 0) {
			perror("timeout");
			continue;
		}

		printf("pds.events=%02x\n", pds.events);
		if (pds.revents & POLLHUP) {
			printf("POLLHUP\n");
			break;
		}

		if (pds.revents & POLLNVAL) {
			printf("POLLNVAL\n");
			break;
		}

		if (pds.revents & POLLOUT) {
			const char *buff = "GET ";
			printf("send \n");
			//ret = send(pds.fd, buff, strlen(buff), 0);
			if (ret == 0) {
				//break;
			}
		}
		if (pds.revents & POLLIN) {
			char buff[100];
			memset(buff, 0, sizeof(buff));
			printf("recv \n");
			ret = recv(pds.fd, buff, sizeof(buff), 0);
			if (ret == 0) {
				close(pds.fd);
				printf("recv err ret=%d\n", ret);
				break;
			}
			printf("poll in buff=%s\n", buff);
		}
		if (pds.revents & POLLERR) {
			printf("poll POLLERR \n");
			break;
		}
		if (i == 65535) {
			shutdown(pds.fd, SHUT_RD);
			printf("close....%d\n", pds.fd);
		}
	}
	printf("...\n");
	return EXIT_SUCCESS;
}
