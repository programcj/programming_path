/*
 * main_server.c
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "list.h"
#include "clog.h"
#include "libemqtt.h"
#include "sockets.h"

//struct mqtt_packet {
//	struct mqtt_packet *next;
//	unsigned char *data;
//	int dlen;
//
//	int pos;
//
//	int command;
//	int rem_length;
//	int rem_byte_size;
//
//	//int rem_pos;
//	int rem_need_length;
//};
//
//struct mqtt_session {
//	struct list_head list;
//
//	int sock;
//	char ip[50];
//	unsigned short port;
//	char clientid[50];
//
//	struct mqtt_packet inpacket;
//	struct mqtt_packet *outpacket;
//	struct mqtt_packet *outpacket_last;
//	struct mqtt_packet *curr_out_packet;
//
//	int heart_time;
//	mqtt_broker_handle_t broker;
//};

void *_thread(void *args) {
	struct sockaddr_in addrto;
	socklen_t client_length;
	int sock_ser = socket(AF_INET, SOCK_DGRAM, 0);
	int ret = 0;
	memset(&addrto, 0, sizeof(addrto));
	addrto.sin_family = AF_INET;
	addrto.sin_port = htons(1884);
	addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);	// htonl(INADDR_BROADCAST); inet_addr(ip);

	char buff[100] = "who is hank wifi device?";

	socket_set_nonblock(sock_ser);
	socket_setopt_broad(sock_ser, 1);

	while (1) {
		memset(&addrto, 0, sizeof(addrto));
		addrto.sin_family = AF_INET;
		addrto.sin_port = htons(1884);
		addrto.sin_addr.s_addr = htonl(INADDR_BROADCAST);// htonl(INADDR_BROADCAST); inet_addr(ip);

		ret = sendto(sock_ser, buff, strlen(buff), 0, (struct sockaddr *) &addrto,
				sizeof(struct sockaddr_in));
		sleep(2);
		client_length = sizeof(struct sockaddr_in);
		ret = recvfrom(sock_ser, buff, sizeof(buff), 0, (struct sockaddr *) &addrto,
				&client_length);
		if (ret > 0) {
			buff[ret] = 0;
			log_d("%s", buff);
		}
	}
	return NULL;
}

struct csession {
	int sock;
	char ip[20];
	unsigned short port;

};

int main_server(int argc, char **argv) {
	int epfd = epoll_create(111);
	int fd_listen = socket_create(SOCK_STREAM, "", 1883);
	int fd_udp = socket_create(SOCK_DGRAM, "", 1884);

	struct epoll_event events[100];
	int nfds;
	int i = 0;

	struct sockaddr_in addr_client;
	socklen_t clilen_len;
	int fd_client;
	struct list_head list_clients;
	int events_count = 2;

	INIT_LIST_HEAD(&list_clients);

	listen(fd_listen, 10);

	epoll_ctl2(epfd, EPOLL_CTL_ADD, fd_listen, EPOLLIN | EPOLLERR | EPOLLHUP, (void*) fd_listen);
	epoll_ctl2(epfd, EPOLL_CTL_ADD, fd_udp, EPOLLIN | EPOLLERR | EPOLLHUP, (void*) fd_udp);

	unsigned char buff[100];
	int ret = 0;

	signal(SIGPIPE, SIG_IGN);

	socket_set_nonblock(fd_udp);

	//pthread_t pt;
	//pthread_create(&pt, NULL, _thread, NULL);

	char tmp[20];

	while (1) {
		nfds = epoll_wait(epfd, events, events_count, 1000 * 10);
		for (i = 0; i < nfds; ++i) {
			if (events[i].events & EPOLLIN) {
				//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
				if (events[i].data.fd == fd_udp) {
					clilen_len = sizeof(addr_client);
					ret = recvfrom(fd_udp, buff, sizeof(buff), 0, (struct sockaddr *) &addr_client,
							&clilen_len);
					if (ret > 0) {
						buff[ret] = 0;
					}
					socket_ntop_ipv4(&addr_client.sin_addr, tmp, sizeof(tmp));

					log_d("socket %d recv from:%s:%d message:[%s],%d bytes/n", fd_udp, tmp,
							ntohs(addr_client.sin_port), buff, ret);
					strcpy((char*) buff, "my is hand wifi device");
					ret = sendto(fd_udp, buff, strlen((char*) buff), 0,
							(struct sockaddr *) &addr_client, clilen_len);
					continue;
				}
				if (events[i].data.fd == fd_listen) {
					clilen_len = sizeof(addr_client);
					fd_client = accept(fd_listen, (struct sockaddr *) &addr_client, &clilen_len);
					socket_set_nonblock(fd_client);
					{
						socket_get_address(fd_client, (char*) buff, sizeof(buff));

						epoll_ctl2(epfd, EPOLL_CTL_ADD, fd_client, EPOLLIN | EPOLLERR | EPOLLRDHUP,
								(void*) fd_client);
						log_d("add client:%d, %s:%d", fd_client, buff, ntohs(addr_client.sin_port));
						events_count++;
					}
					continue;
				}
				//------------------------------------
				{
					fd_client = events[i].data.fd;
					ret = recv(fd_client, buff, 100, 0);
					if (ret <= 0) {
						epoll_ctl2(epfd, EPOLL_CTL_DEL, fd_client, 0, 0);

						shutdown(fd_client, SHUT_RDWR);
						close(fd_client);

						events_count--;
						log_d("client-close:%d", fd_client);
						continue;
					}
					log_hex(buff, ret, "recv:%d: ", fd_client);

					switch (MQTTParseMessageType(buff)) {
					case MQTT_MSG_CONNECT: {
						buff[0] = (unsigned char) MQTT_MSG_CONNACK;
						buff[1] = 0x02;
						buff[2] = 0x00;
						buff[3] = 0x00;
						ret = send(fd_client, buff, 4, 0);
						log_hex(buff, 4, "%d,MQTT_MSG_CONNACK send:%d:", fd_client, ret);
					}
						break;
					case MQTT_MSG_PUBLISH: {
						if (MQTTParseMessageQos(buff) == 1) { //QOS==1需要发送ack
							int msg_id = mqtt_parse_msg_id(buff);

							buff[0] = MQTT_MSG_PUBACK;
							buff[1] = 0x02;
							buff[2] = (msg_id & 0xFF00) >> 8;
							buff[3] = (msg_id & 0x00FF);
							ret = send(fd_client, buff, 4, 0);
							log_hex(buff, 4, "%d,MQTT_MSG_PUBACK, QOS1=%d send:(%d):", fd_client,
									msg_id, ret);
						}
					}
						break;
					}
				}
			}
			if (events[i].events & EPOLLOUT) // 如果有数据发送
			{

			}
			if (events[i].events & EPOLLERR) {
				epoll_ctl2(epfd, EPOLL_CTL_DEL, fd_client, 0, 0);
				shutdown(fd_client, SHUT_RDWR);
				close(fd_client);
				events_count--;
				log_d("client-close:%d: ", fd_client);
			} //客户端有事件到达
			if (events[i].events & EPOLLHUP) {
				log_d("EPOLLHUP");
			}
		}
	}

	return 0;
}
