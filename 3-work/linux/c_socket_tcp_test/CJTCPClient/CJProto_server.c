//
//  CJProto_server.c
//  CJTCPClient
//
//  Created by   CC on 2017/12/2.
//  Copyright © 2017年 CC. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <string.h>

#include "CJProto_server.h"
#include "list.h"
#include "clog.h"

//EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//EPOLLOUT：表示对应的文件描述符可以写；
//EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//EPOLLERR：表示对应的文件描述符发生错误；
//EPOLLHUP：表示对应的文件描述符被挂断；
//EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
//EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int epoll_add(int epfd, int fd, int events) {
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = events; //设置要处理的事件类型
	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

int epoll_del(int epfd, int fd, int events) {
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = events; //设置要处理的事件类型
	return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
}

int epoll_mod(int epfd, int fd, int events) {
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = events; //设置要处理的事件类型
	return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

int fd_setnonblock(int fd) {
	int opt;
	/* Set non-blocking */
	opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1) {
		close(fd);
		return 1;
	}
	if (fcntl(fd, F_SETFL, opt | O_NONBLOCK) == -1) {
		/* If either fcntl fails, don't want to allow this client to connect. */
		close(fd);
		return 1;
	}
	return 0;
}

int list_size(struct list_head *list) {
	struct list_head *p = NULL;
	int size = 0;
	list_for_each(p, list)
		size++;
	return size;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct cjprotot_packet {
	struct cjprotot_packet *next;
	int type;  //接收的时候,依次接收
	int remlength; //接收的时候,依次接收,使用remlength_size确定接收其个数
	/*发送时,须要添加两字节做数据头,哪么，发送的时候只须要发送此数据就可
	 接收的时候须要remreadlen确定接收个数, rempos确定接收当前位置; */
	unsigned char *remdata;
	int remlength_size; //确定接收时remlength是否收完
	int remreadlen; //接收数据大小确定，发送大小确定(是否收完/是否发完)
	int rempos;  //接收/发送，确定发送起点，或接收起点
};

struct cjproto_server {
	struct list_head list_clients;
	int list_clientsize;
	int epfd;
};

struct cjproto_server server_main;

struct cjproto_client {
	struct list_head list;
	int sock;

	struct cjprotot_packet in_packet;
	struct cjprotot_packet *out_packet;
	struct cjprotot_packet *out_packet_end;
	struct cjprotot_packet *cur_packet;
};

void client_close(struct cjproto_client *client, struct cjproto_server *server) {
	if (client->sock != INVALID_SOCKET) {
		list_del(&client->list);
		server->list_clientsize--;
		epoll_del(server->epfd, client->sock, EPOLLIN);
		close(client->sock);
		client->sock = -1;
	}
	free(client);
}

int loop_handle_accept(int fd, struct cjproto_server *server) {
	struct cjproto_client *client = NULL;
	int newsock = accept(fd, NULL, 0);
	if (newsock == INVALID_SOCKET)
		return -1;
	if (fd_setnonblock(newsock) != 0) {
		return -1;
	}
	client = (struct cjproto_client *) realloc(client,
			sizeof(struct cjproto_client));
	if (!client) {
		close(newsock);
		return -1;
	}
	memset(client, 0, sizeof(struct cjproto_client));
	client->sock = newsock;
	list_add_tail(&client->list, &server->list_clients);
	server->list_clientsize++;
	return newsock;
}

int loop_handle_write(struct cjproto_client *client) {
	struct cjprotot_packet *packet = NULL;
	int wlen = 0;
	uint8_t byte;
	packet = client->out_packet;

	log_d("write>>>>");
	while (packet) {
		if (packet->type != 0) {
			wlen = write(client->sock, &packet->type, 1);
			if (wlen == 0)
				return -1;
			if (wlen < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					log_d("||||||||||||||||||||||||||||||||||||||||||");
					return 0;
				} else {
					return -1;
				}
			}
			packet->type = 0;
		}
		while (packet->remlength > 0) {
			byte = packet->remlength & 0x7F;
			if ((packet->remlength & 0x80) > 0) {
				byte |= 0x80;
			}
			wlen = write(client->sock, &byte, 1);
			if (wlen == 1) {
				//packet->remlength_size--;
				packet->remlength >>= 7;
			}
			if (wlen == 0)
				return -1;
			if (wlen < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					log_d("||||||||||||||||||||||||||||||||||||||||||");
					return 0;
				} else {
					return -1;
				}
			}
		}
		if (packet->remdata)
			free(packet->remdata);
		free(packet);

		if (client->out_packet) {
			client->out_packet = client->out_packet->next;
			if (client->out_packet == NULL) {
				client->out_packet_end = NULL;
			}
		}
		packet = client->out_packet;
	}
	return 0;
}

int loop_handle_read(struct cjproto_client *client) {
	int rlen = 0;
	uint8_t byte;
	int sockfd = client->sock;
	if (!client)
		return -1;

	if (client->in_packet.type == 0) {
		rlen = read(sockfd, &byte, 1);
		if (rlen == 1) {
			client->in_packet.type = byte;
		}
		if (rlen == 0)
			return -1;
		if (rlen < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	if (client->in_packet.remlength_size <= 0) {
		do {
			rlen = read(sockfd, &byte, 1);
			if (rlen == 1) {
				client->in_packet.remlength_size--;
				switch (client->in_packet.remlength_size) {
				case -1:
					client->in_packet.remlength = (byte & 0x7F);
					break;
				case -2:
					client->in_packet.remlength = ((byte & 0x7F) << 7)
							| client->in_packet.remlength;
					break;
				case -3:
					client->in_packet.remlength = ((byte & 0x7F) << 7 * 2)
							| client->in_packet.remlength;
					break;
				case -4:
					client->in_packet.remlength = ((byte & 0x7F) << 7 * 3)
							| client->in_packet.remlength;
					break;
				}
				if (client->in_packet.remlength_size <= -4) {
					client->in_packet.remlength_size *= -1; // -1*-4=4
				}
			}
			if (rlen == 0)
				return -1;
			if (rlen < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					return 0;
				} else {
					return -1;
				}
			}
		} while (((byte & 0x80) != 0) && (client->in_packet.remlength_size <= 0));

		client->in_packet.remdata = malloc(client->in_packet.remlength);
		if (!client->in_packet.remdata)
			return -1;
		client->in_packet.remreadlen = client->in_packet.remlength;
		client->in_packet.rempos = 0;
	}

	while (client->in_packet.remreadlen > 0) {
		rlen = read(sockfd,
				client->in_packet.remdata + client->in_packet.rempos,
				client->in_packet.remreadlen);
		if (rlen > 0) {
			client->in_packet.rempos += rlen;
			client->in_packet.remreadlen -= rlen;
		}
		if (rlen == 0)
			return -1;
		if (rlen < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	log_hex(client->in_packet.remdata, client->in_packet.remlength,
			"type=%02x, len=%d>>>>>", client->in_packet.type,
			client->in_packet.remlength);

	if (client->in_packet.type == 0x10) { //connect
		struct cjprotot_packet *out_packet = (struct cjprotot_packet *) malloc(
				sizeof(struct cjprotot_packet));
		if (out_packet) {
			memset(out_packet, 0, sizeof(struct cjprotot_packet)); //发送的时候把整个数据包生成到remdata里面
			out_packet->type = 0x20;
			out_packet->remlength = 0x2F;
			out_packet->remlength_size = 1;

			if (!client->out_packet)
				client->out_packet = out_packet;
			else
				client->out_packet_end->next = out_packet;
			client->out_packet_end = out_packet;
			rlen = loop_handle_write(client);
			log_d("write>>>>%d", rlen);
		}
	}

	if (client->in_packet.remdata)
		free(client->in_packet.remdata);
	memset(&client->in_packet, 0, sizeof(struct cjprotot_packet));
	return -1;

//epoll_mod(server->epfd, sockfd, EPOLLOUT);
//return 0;
}

int loop_handle_rw(struct cjproto_server *server, struct epoll_event *event) {
	struct cjproto_client *client = NULL;
	int sockfd = event->data.fd;
	struct list_head *p = NULL;

	if (sockfd <= 0)
		return -1;

	list_for_each(p, &server->list_clients)
	{
		client = list_entry(p, struct cjproto_client, list);
		if (client->sock == sockfd) {
			break;
		}
		client = NULL;
	}

	if (event->events & EPOLLIN) {
		if (loop_handle_read(client)) {
			client_close(client, server);
		}
	} else if (event->events & EPOLLOUT) {

		//					int rlen = write(sockfd, "hello ", 6);
		//					if (rlen <= 0) {
		//						struct cjproto_client *client = NULL;
		//						struct list_head *p = NULL;
		//						list_for_each(p, &server->list_clients)
		//						{
		//							client = list_entry(p, struct cjproto_client, list);
		//							if (client->sock == sockfd) {
		//								client_close(client, server);
		//							}
		//						}
		//					} else {
		//						epoll_mod(server->epfd, sockfd, EPOLLIN);
		//					}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int loop(void) {
	struct epoll_event *events = NULL;
	int events_size = 0, epwsize = 0;
	int sock;
	struct sockaddr_in serveraddr;
	int sock_client;
	int i = 0;
	int client_size = 0;
	int ret = 0;

	struct cjproto_server *server = &server_main;
	memset(server, 0, sizeof(struct cjproto_server));
	INIT_LIST_HEAD(&server->list_clients);

	server->epfd = epoll_create1(0);
	sock = socket(AF_INET, SOCK_STREAM, 0);

	epoll_add(server->epfd, sock, EPOLLIN); //EPOLLIN | EPOLLET;

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	char *local_addr = "0.0.0.0";
	inet_aton(local_addr, &(serveraddr.sin_addr)); //htons(portnumber);
	serveraddr.sin_port = htons(1883);

	ret = bind(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
	if (ret < 0) {
		perror("bind err..");
		return -1;
	}
	ret = listen(sock, 150);
	if (ret < 0) {
		perror("listen err..");
		return -1;
	}
	sigset_t sigblock; 	//sigset_t sigblock, origsig;
	sigemptyset(&sigblock);
	sigaddset(&sigblock, SIGINT);

	log_d("start...");
	while (1) {
		client_size = 1 + server->list_clientsize;
		if (!events || events_size < client_size) {
			events = realloc(events,
					client_size * (sizeof(struct epoll_event)));
			if (!events) {
				log_e("sys no mem:%s", strerror(errno));
				break;
			}
			events_size = client_size;
		}
		memset(events, -1, sizeof(struct epoll_event) * client_size);
		epwsize = epoll_wait(server->epfd, events, client_size, 500);

		for (i = 0; i < epwsize; i++) {
			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
					|| (!(events[i].events & EPOLLIN)
							&& !(EPOLLOUT & events[i].events))) {
				log_d("epoll errl....＞%X", events[i].events);
				log_d("events[i].events & EPOLLERR=%X",
						events[i].events & EPOLLERR);
				log_d("events[i].events & EPOLLHUP=%x",
						events[i].events & EPOLLHUP);
				log_d("events[i].events & EPOLLIN=%x",
						events[i].events & EPOLLIN);
				exit(1);
			} else if (events[i].data.fd == sock) {

				if (events[i].events & (EPOLLIN | EPOLLPRI)) {
					sock_client = loop_handle_accept(sock, server);
					if (sock_client > 0) {
						epoll_add(server->epfd, sock_client, EPOLLIN); //ev.events = EPOLLIN | EPOLLET;
					}
					log_d("_clientsize:%d", server->list_clientsize);
				} else {
					log_e("=============================================");
				}
			} else {
				loop_handle_rw(server, &events[i]);
			}
		}
	}
	return EXIT_SUCCESS;
}

void CJProto_server_loop() {
	loop();
}
