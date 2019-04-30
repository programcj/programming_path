/*
 * sockets.h
 *
 *  Created on: 2018年4月7日
 *      Author: cj
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

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

int socket_set_nonblock(int sock);

int socket_setopt_broad(int sock, int flag);

int socket_setopt_reuseaddr(int sock, int flag);

#define sock_create_tcp(host, port)   	sock_create(SOCK_STREAM,host, port)
#define sock_create_udp(host, port)   sock_create(SOCK_DGRAM,host, port)

int socket_create(int style, const char *host, unsigned short port);

/**
 * @__op:  EPOLL_CTL_ADD/EPOLL_CTL_DEL/EPOLL_CTL_MOD
 * @events:  [enum EPOLL_EVENTS]
 */
int epoll_ctl2(int epfd, int __op, int _fd, uint32_t events, void *ptr);

int socket_get_address(int sock, char *buff, int len);

const char *socket_ntop_ipv4(struct in_addr *addr, char *buff, socklen_t len);

void sockaddr_in_init(struct sockaddr_in *addr, int sin_family, unsigned short port, const char *ip);

#include <errno.h>

int socket_send(int sock, void *buffer, int size);
int socket_recv(int sock, void *bufer, int size);

#endif /* SOCKETS_H_ */
