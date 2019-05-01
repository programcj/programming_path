/*
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

#define SUPPORT_EPOLL

struct mqtt_packet {
	struct mqtt_packet *next;
	unsigned char *data;
	int dlen;
	int pos;
	int command;
	int rem_length;
	int rem_byte_size;

	//int rem_pos;
	int rem_need_length;
};

struct mqtt_session {
	struct list_head list;

	int sock;
	char ip[50];
	unsigned short port;
	char clientid[50];

	struct mqtt_packet inpacket;
	struct mqtt_packet *outpacket;
	struct mqtt_packet *outpacket_last;
	struct mqtt_packet *curr_out_packet;

	int heart_time;
	int last_time; //最后通信时间
	mqtt_broker_handle_t broker;

#ifdef SUPPORT_EPOLL
	int epfd;
#endif

};

void *_thread1(void *args) {
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

int _process_recvfrom(int sock) {
	struct sockaddr_in addr_client;
	socklen_t clilen_len;
	int ret = 0;
	unsigned char buff[100];
	char tmp[30];

	clilen_len = sizeof(addr_client);
	ret = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *) &addr_client, &clilen_len);
	if (ret > 0) {
		buff[ret] = 0;
	}
	socket_ntop_ipv4(&addr_client.sin_addr, tmp, sizeof(tmp));
//	log_d("socket %d recv from:%s:%d message:[%s],%d bytes/n", sock, tmp,
//			ntohs(addr_client.sin_port), buff, ret);

	//strcpy((char*) buff, "my is hand wifi device");
	ret = sendto(sock, buff, strlen((char*) buff), 0, (struct sockaddr *) &addr_client, clilen_len);

	return atoi((char*) buff);
}

static int mqtt_session_push_packet(void* socket_info, const void* buff, unsigned int count) {
	struct mqtt_session *session = (struct mqtt_session*) socket_info;
	struct mqtt_packet *packet = (struct mqtt_packet*) malloc(sizeof(struct mqtt_packet));
	if (!packet)
		return -1;

	memset(packet, 0, sizeof(struct mqtt_packet));
	packet->data = malloc(count);

	if (!packet->data) {
		free(packet);
		return -1;
	}

	memcpy(packet->data, buff, count);
	packet->dlen = count;
	if (session->outpacket == NULL) {
		session->outpacket = packet;
		session->outpacket_last = packet;
	} else {
		session->outpacket_last->next = packet;
		session->outpacket_last = packet;
	}

	epoll_ctl2(session->epfd, EPOLL_CTL_MOD, session->sock, EPOLLOUT, session);
	return 0;
}

struct mqtt_session *client_accept(int fd_listen, int epfd, struct list_head *clients) {
	struct mqtt_session *session = NULL;
	struct sockaddr_in addr_client;
	socklen_t clilen_len;
	int fd_client = 0;
	char buff[20];

	clilen_len = sizeof(addr_client);
	fd_client = accept(fd_listen, (struct sockaddr *) &addr_client, &clilen_len);
	socket_set_nonblock(fd_client);
	socket_get_address(fd_client, (char*) buff, sizeof(buff));

	session = (struct mqtt_session*) malloc(sizeof(struct mqtt_session));
	if (session == NULL) {
		close(fd_client);
		return NULL;
	}
	memset(session, 0, sizeof(struct mqtt_session));

	strcpy(session->ip, buff);
	session->port = ntohs(addr_client.sin_port);
	session->sock = fd_client;

	session->broker.socket_info = session;
	session->broker.send = mqtt_session_push_packet;

	session->heart_time = 50;
	session->last_time = time(NULL);
	log_d("add client:%d, %s:%d", session->sock, session->ip, session->port);

	list_add(&session->list, clients);

	session->epfd = epfd;
	return session;
}

int client_read_message(struct mqtt_session *session) {
	struct mqtt_packet *pack = &session->inpacket;
	int sock = session->sock;
	unsigned char tmpbyte;
	int rc = 0;

	if (!pack->command) {
		rc = read(sock, &tmpbyte, 1);
		if (rc == 1) {
			pack->command = tmpbyte;
//			pack->data[pack->pos++] = tmpbyte;
		} else if (rc == 0)
			return -1;
		else {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return 0;
			} else {
				return -1;
			}
		}
	}

	if (pack->rem_byte_size <= 0) {
		do {
			rc = read(sock, &tmpbyte, 1);
			if (rc == 1) { //小端模式129(0x81)=(0x81,0x01), 地位在前
//				pack->data[pack->pos++] = tmpbyte;
				pack->rem_length |= (tmpbyte & 0x7F) << (7 * (pack->rem_byte_size * -1));
				pack->rem_byte_size--;
				if (pack->rem_byte_size <= -4 || (tmpbyte & 0x80) == 0x00) {
					pack->rem_byte_size *= -1;
				}
			} else if (rc == 0) {
				rc = -1;
			} else if (errno == EWOULDBLOCK || errno == EAGAIN) {
				rc = 0;
			} else {
				rc = -1;
			}
		} while (pack->rem_byte_size <= 0 && rc == 1);

		if (rc == -1)
			return rc;
		log_hex(&pack->rem_length, 4, "packet_length:");

		pack->pos = 1 + pack->rem_byte_size;
		pack->dlen = 1 + pack->rem_byte_size + pack->rem_length;
		pack->data = realloc(pack->data, pack->dlen);

		pack->data[0] = pack->command;
		mqtt_remainlen2bytes(pack->rem_length, pack->data + 1);

		pack->rem_need_length = pack->rem_length;
		//pack->pos = 0;
	}

	while (pack->rem_need_length > 0) {
		if (pack->pos + pack->rem_need_length > pack->dlen) {
			return -402;
		}
		rc = read(sock, pack->data + pack->pos, pack->rem_need_length);
		if (rc > 0) {
			pack->rem_need_length -= rc;
			pack->pos += rc;
		} else if (rc == 0) {
			rc = -1;
		} else if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return 0;
		} else {
			return -1;
		}
	}

	log_hex(pack->data, pack->pos, "recv-mqtt:");
	{
		unsigned char *buff = pack->data;
		int ret = 0;
		int fd_client = session->sock;
		switch (MQTTParseMessageType(buff)) {
		case MQTT_MSG_CONNECT: {
			buff[0] = (unsigned char) MQTT_MSG_CONNACK;
			buff[1] = 0x02;
			buff[2] = 0x00;
			buff[3] = 0x00;

			mqtt_session_push_packet(session, buff, 4);
			log_d("MQTT_MSG_CONNACK");
			//ret = send(fd_client, buff, 4, 0);
		}
			break;
		case MQTT_MSG_PUBLISH: {
			if (MQTTParseMessageQos(buff) == 1) { //QOS==1需要发送ack
				int msg_id = mqtt_parse_msg_id(buff);

				buff[0] = MQTT_MSG_PUBACK;
				buff[1] = 0x02;
				buff[2] = (msg_id & 0xFF00) >> 8;
				buff[3] = (msg_id & 0x00FF);

				mqtt_session_push_packet(session, buff, 4);
				log_d("MQTT_MSG_PUBACK(QOS=1)");
			}
			char *topic = NULL;
			char *msg = NULL;

			uint8_t *topic_ptr = 0;
			uint16_t topic_len = mqtt_parse_pub_topic_ptr(buff, &topic_ptr);

			uint8_t *msg_ptr;
			uint16_t msg_len = mqtt_parse_pub_msg_ptr(buff, &msg_ptr);

			topic = calloc(topic_len + 1, 1);
			memcpy(topic, topic_ptr, topic_len);

			if (msg_len > 0) {
				msg = calloc(msg_len + 1, 1);
				memcpy(msg, msg_ptr, msg_len);
			}
			log_d("Topic:%s, msg:%s", topic, msg);
			if (topic)
				free(topic);
			if (msg)
				free(msg);
		}
			break;
		case MQTT_MSG_SUBSCRIBE: {
			int msg_id = mqtt_parse_msg_id(buff);
			buff[0] = MQTT_MSG_SUBACK;
			buff[1] = 3;
			buff[2] = (msg_id & 0xFF00) >> 8;
			buff[3] = (msg_id & 0x00FF);
			buff[4] = 0x01;
			mqtt_session_push_packet(session, buff, 5);
		}
			break;
		case MQTT_MSG_PINGREQ: {
			buff[0] = MQTT_MSG_PINGRESP;
			buff[1] = 0;
			mqtt_session_push_packet(session, buff, 2);
			log_d("MQTT_MSG_PINGRESP");
		}
			break;
		}
	}
	pack->pos = 0;
	pack->command = 0;
	pack->rem_byte_size = 0;
	pack->rem_length = 0;
	pack->rem_need_length = 0;
	return 0;
}

int client_push_msg(struct mqtt_session *session) {
	int ret = 0;

	if (session->curr_out_packet) {
		ret = send(session->sock, session->curr_out_packet->data + session->curr_out_packet->pos,
				session->curr_out_packet->dlen - session->curr_out_packet->pos, 0);
		if (ret > 0) {
			session->curr_out_packet->pos += ret;
			if (session->curr_out_packet->pos >= session->curr_out_packet->dlen) {
				session->outpacket = session->outpacket->next; //

				log_hex(session->curr_out_packet->data, session->curr_out_packet->dlen, "send:");
				free(session->curr_out_packet->data);
				free(session->curr_out_packet);
				session->curr_out_packet = NULL;
			}
			return 0;
		}
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return 0;
		}
	}
	return -1;
}

void client_disconnect(struct mqtt_session *session) {
	struct mqtt_packet *pack = NULL;

	if (session->sock != -1) {
		shutdown(session->sock, SHUT_RDWR);
		close(session->sock);
		session->sock = -1;
	}

	list_del(&session->list);

	if (session->inpacket.data) {
		free(session->inpacket.data);
	}

	while (session->outpacket) {
		if (session->outpacket->data)
			free(session->outpacket->data);
		pack = session->outpacket;
		session->outpacket = pack->next;
		free(pack);
	}
	free(session);
}

int main(int argc, char **argv) {
	struct mqtt_session *session = NULL;

	int epfd = epoll_create1(0);
	int fd_listen = socket_create(SOCK_STREAM, "", 1883);
	int fd_udp = socket_create(SOCK_DGRAM, "", 1884);

	int time_cur;
	struct epoll_event events[100];
	int nfds;
	int i = 0;

	struct list_head list_clients;
	int events_count = 2;

	INIT_LIST_HEAD(&list_clients);

	listen(fd_listen, 10);

	epoll_ctl2(epfd, EPOLL_CTL_ADD, fd_listen, EPOLLIN | EPOLLERR | EPOLLHUP, (void*) &fd_listen);
	epoll_ctl2(epfd, EPOLL_CTL_ADD, fd_udp, EPOLLIN | EPOLLERR | EPOLLHUP, (void*) &fd_udp);

	signal(SIGPIPE, SIG_IGN);

	socket_set_nonblock(fd_udp);

	//pthread_t pt;
	//pthread_create(&pt, NULL, _thread1, NULL);

	log_d("run");

	while (1) {
		time_cur = time(NULL);

		{	//time out check
			struct list_head *pos, *n;

			list_for_each_safe(pos,n, &list_clients)
			{
				session = list_entry(pos, struct mqtt_session, list);
				if (time_cur - session->last_time > session->heart_time) {
					log_d("time out...");
					epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
					client_disconnect(session);
					session = NULL;
					events_count--;
				}
			}
		}
		nfds = epoll_wait(epfd, events, events_count, 1000 * 10);

		for (i = 0; i < nfds; ++i) {

			if (events[i].events & EPOLLIN) {		//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
				//UDP
				if (events[i].data.ptr == &fd_udp) {

					switch (_process_recvfrom(fd_udp)) {
					case 0:
						break;
					case 1:
						log_d("clients:%d", events_count - 2);
						{
							int count = 0;
							struct list_head *pos, *n;
							list_for_each_safe(pos,n, &list_clients)
							{
								count++;
							}
							log_d("client-count:%d", count);
						}
						break;
					case 2:
						break;
					case 3:
						break;
					default:
						break;
					}
					continue;
				}
				//TCP_server
				if (events[i].data.ptr == &fd_listen) {
					session = client_accept(fd_listen, epfd, &list_clients);
					if (session) {
						epoll_ctl2(epfd, EPOLL_CTL_ADD, session->sock,
						EPOLLIN | EPOLLERR | EPOLLRDHUP, (void*) session);
						events_count++;
					}
					continue;
				}
				//client recv msg
				{
					session = events[i].data.ptr;
					session->last_time = time(NULL);

					if (client_read_message(session)) {
						log_d("read message err.");
						epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
						client_disconnect(session);
						session = NULL;
						events_count--;
					}
				}
			}

			if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				if (events[i].data.ptr != &fd_listen && events[i].data.ptr != &fd_udp) {
					session = events[i].data.ptr;
					session->last_time = time(NULL);

					if (session->curr_out_packet == NULL) {
						session->curr_out_packet = session->outpacket;
					}

					if (client_push_msg(session) == 0) {
						if (session->outpacket == NULL)
							epoll_ctl2(session->epfd, EPOLL_CTL_MOD, session->sock,
							EPOLLIN | EPOLLERR | EPOLLRDHUP, session);
					} else {
						epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
						client_disconnect(session);
						events_count--;
						session = NULL;
					}
				}
			}
			if (events[i].events & EPOLLERR) {
				if (events[i].data.ptr != &fd_listen && events[i].data.ptr != &fd_udp) {
					session = events[i].data.ptr;

					epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
					client_disconnect(session);
					session = NULL;
					events_count--;
				}
				log_d("EPOLLERR");
			} //客户端有事件到达
			if (events[i].events & EPOLLHUP) {
				log_d("EPOLLHUP");
			}
		}
	}

	return 0;
}
