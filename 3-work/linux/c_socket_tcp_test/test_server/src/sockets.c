/*
 * sockets.c
 *
 *  Created on: 2018年4月7日
 *      Author: cj
 */

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include<arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int socket_set_nonblock(int sock) {
	int opt;
	opt = fcntl(sock, F_GETFL, 0);
	if (opt == -1) {
		close(sock);
		return 1;
	}
	if (fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1) {
		close(sock);
		return 1;
	}
	return 0;
}

int socket_setopt_broad(int sock, int flag) {
	return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *) &flag, sizeof(flag));
}

int socket_setopt_reuseaddr(int sock, int flag) {
	return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(flag));
}

#define sock_create_tcp(host, port)   	sock_create(SOCK_STREAM,host, port)
#define sock_create_udp(host, port)   sock_create(SOCK_DGRAM,host, port)

int socket_create(int style, const char *host, unsigned short port) {
	int sock = 0;
	struct sockaddr_in addr;
	sock = socket(AF_INET, style, 0);
	if (sock < 0)
		return -1;

	bzero(&addr, sizeof(addr)); //
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);

	if (socket_setopt_reuseaddr(sock, 1) < 0) {
		close(sock);
		return -1;
	}

	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		close(sock);
		return -1;
	}
	return sock;
}

/**
 * @__op:  EPOLL_CTL_ADD/EPOLL_CTL_DEL/EPOLL_CTL_MOD
 * @events:  [enum EPOLL_EVENTS]
 */
int epoll_ctl2(int epfd, int __op, int _fd, uint32_t events, void *ptr) {
//	EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//	EPOLLOUT：表示对应的文件描述符可以写；
//	EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//	EPOLLERR：表示对应的文件描述符发生错误；
//	EPOLLHUP：表示对应的文件描述符被挂断；
//	EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
//	EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = ptr;
	return epoll_ctl(epfd, __op, _fd, &ev);
}

int socket_get_address(int sock, char *buff, int len) {
	struct sockaddr_storage addr;
	socklen_t addrlen;

	addrlen = sizeof(addr);
	if (!getpeername(sock, (struct sockaddr *) &addr, &addrlen)) {
		if (addr.ss_family == AF_INET) {
			if (inet_ntop(AF_INET, &((struct sockaddr_in *) &addr)->sin_addr.s_addr, buff, len)) {
				return 0;
			}
		} else if (addr.ss_family == AF_INET6) {
			if (inet_ntop(AF_INET6, &((struct sockaddr_in6 *) &addr)->sin6_addr.s6_addr, buff,
					len)) {
				return 0;
			}
		}
	}
	return 1;
}

const char *socket_ntop_ipv4(struct in_addr *addr, char *buff, socklen_t len) {
	return inet_ntop(AF_INET, addr, buff, len);
}

void sockaddr_in_init(struct sockaddr_in *addr, int sin_family, unsigned short port, const char *ip) {
	addr->sin_family = AF_INET;
	addr->sin_port = htons(1884);
	addr->sin_addr.s_addr = inet_addr(ip); // htonl(INADDR_BROADCAST);	// htonl(INADDR_BROADCAST); inet_addr(ip);
}
#include <errno.h>

int socket_send(int sock, void *buffer, int size) {
	int ret = 0;
	int pos = 0;

	while (pos < size) {
		ret = send(sock, buffer + pos, size - pos, 0);
		if (ret > 0)
			pos += size;
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			return -1;
		}
		if (ret == 0)
			return -1;
	}
	return pos;
}

int socket_recv(int sock, void *bufer, int size) {
	int ret = 0;
	int pos = 0;

	while (pos < size) {
		ret = recv(sock, bufer + pos, size - pos, 0);
		if (ret > 0) {
			pos += ret;
		}
		if (ret == 0)
			return -1; //CONN_LOST
		if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				switch (errno) {
				case ECONNRESET:
					return -1; //ERR_CONN_LOST;
				default:
					return -1;
				}
			}
		}
	}
	return pos;
}

int socket_pair(int fds[2]) {
	int ret = 0;
	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
	return ret;
}

#include <sys/un.h>

//need: unlink(path);
int socket_create_unix(const char *path) {
	int fd = 0;
	struct sockaddr_un addr;
	int addr_len = 0;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		return -1;
	}
	addr_len = sizeof(addr);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);

	if (bind(fd, (struct sockaddr *) &addr, addr_len) < -1) {
		close(fd);
		return -1;
	}
	return fd;
}
