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

#include "clog.h"
#include "sockets.h"
#include "list.h"

#include "mem.h"

#define malloc(size)	mem_malloc(size)
#define free(ptr)			mem_free(ptr)

#define SUPPORT_EPOLL

#define MSG_BODY_TYPE_FILE		1  //文件类型的数据包

struct head_pack {
	unsigned char _pack_head; //数据包头
	unsigned short _msg_len; //消息头
	int _msg_len_size; //消息头长度(2Byte)
	unsigned char* _head_msg; //消息头消息
	int _msg_pos;
};

struct body_file {
	FILE *fp;
	char file_name[30];
	unsigned long file_size;
	unsigned long pos;
};

struct cpacket {
	struct cpacket *next;
	unsigned char *data;
	int dlen;
	int pos;
};

struct tcp_client {
	struct list_head list;

	int sock;
	char ip[50];
	unsigned short port;

	struct head_pack hpack;
	int hpack_body_type; //hpack 是否读取完成

	struct body_file file;

	struct cpacket *packet_cur;
	struct cpacket *packet_head;
	struct cpacket *packet_last;

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
	char info[200];
	char tmp[30];

	clilen_len = sizeof(addr_client);
	ret = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *) &addr_client, &clilen_len);
	if (ret > 0) {
		buff[ret] = 0;
	}
	socket_ntop_ipv4(&addr_client.sin_addr, tmp, sizeof(tmp));

	if (strncmp(buff, "HK/1", 4) == 0) {
		sprintf(buff, "HK/1/0.0.0.0/");
		ret = sendto(sock, buff, strlen((char*) buff), 0, (struct sockaddr *) &addr_client,
				clilen_len);
	}

	return atoi((char*) buff);
}

struct tcp_client *client_accept(int fd_listen, int epfd, struct list_head *clients) {
	struct tcp_client *session = NULL;
	struct sockaddr_in addr_client;
	socklen_t clilen_len;
	int fd_client = 0;
	char buff[20];

	clilen_len = sizeof(addr_client);
	fd_client = accept(fd_listen, (struct sockaddr *) &addr_client, &clilen_len);
	socket_set_nonblock(fd_client);
	socket_get_address(fd_client, (char*) buff, sizeof(buff));

	session = (struct tcp_client*) malloc(sizeof(struct tcp_client));
	if (session == NULL) {
		close(fd_client);
		return NULL;
	}
	memset(session, 0, sizeof(struct tcp_client));

	strcpy(session->ip, buff);
	session->port = ntohs(addr_client.sin_port);
	session->sock = fd_client;

	log_d("add client:%d, %s:%d", session->sock, session->ip, session->port);

	list_add(&session->list, clients);

	session->epfd = epfd;
	return session;
}

int client_read_message_headpack(struct tcp_client *session) {
	int sock = session->sock;
	unsigned char tmpbyte;
	int rc = 0;
	struct head_pack *hpack = &session->hpack;

	if (session->hpack_body_type == 0) {
		////
		if (!hpack->_pack_head) {
			rc = read(sock, &tmpbyte, 1);
			if (rc == 1) {
				hpack->_pack_head = tmpbyte;
			} else {
				if (rc == 0)
					return -1;
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					return 0;
				return -1;
			}
		}

		if (hpack->_msg_len_size <= 0) {
			do {
				rc = read(sock, &tmpbyte, 1);
				if (rc == 1) { //小端模式129(0x81)=(0x81,0x01), 地位在前
					hpack->_msg_len <<= 8;
					hpack->_msg_len |= tmpbyte;

					hpack->_msg_len_size--;
					if (hpack->_msg_len_size <= -2) {
						hpack->_msg_len_size *= -1;
					}
				} else {
					if (rc == 0)
						return -1;
					if (errno == EWOULDBLOCK || errno == EAGAIN)
						return 0;
					return -1;
				}
			} while (hpack->_msg_len_size <= 0 && rc == 1);

			if (rc == -1)
				return rc;

			log_d("msg len:%d(0x%X)", hpack->_msg_len, hpack->_msg_len);
			hpack->_head_msg = (unsigned char*) malloc(hpack->_msg_len);
			hpack->_msg_pos = 0;
		}

		while (hpack->_msg_pos < hpack->_msg_len) {
			rc = read(sock, hpack->_head_msg + hpack->_msg_pos, hpack->_msg_len - hpack->_msg_pos);
			if (rc > 0) {
				hpack->_msg_pos += rc;
			} else {
				if (rc == 0)
					return -1;
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					return 0;
				return -1;
			}
		}
		session->hpack_body_type = 1;
		////

		if (session->hpack._msg_len > 0) {
			if (session->hpack._head_msg[0] == 1) {
				strncpy(session->file.file_name, (char*) session->hpack._head_msg + 1, 30);
				memcpy(&session->file.file_size, session->hpack._head_msg + 1 + 30, 4);
				session->file.pos = 0;

				log_d("固件文件写入开始: name:%s, size:%d", session->file.file_name,
						session->file.file_size);

				free(session->hpack._head_msg);
				session->hpack._head_msg = NULL;
				session->hpack._msg_len = 0;
				session->hpack._msg_len_size = 0;
				session->hpack._msg_pos = 0;
				session->hpack._pack_head = 0;

				session->file.fp = fopen("/tmp/frame.bin", "wb");
			}
		}
	}
	if (session->hpack_body_type == 1) { //数据包头接收完成
		unsigned char buff[100];
		int ret = 0;
		unsigned long need_size = 0;

		while (session->file.pos < session->file.file_size) {
			need_size = session->file.file_size - session->file.pos;
			if (need_size < sizeof(buff)) {
				ret = recv(sock, buff, sizeof(buff) - need_size, 0);
			} else {
				ret = recv(sock, buff, sizeof(buff), 0);
			}

			if (ret > 0) {
				fwrite(buff, 1, ret, session->file.fp);
				session->file.pos += ret;
			} else {
				log_d("session->file.pos=%d", session->file.pos);

				if (rc == 0)
					return -1;
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					return 0;
				return -1;
			}
		}
		log_d("file read end:%d", session->file.pos);
		fclose(session->file.fp);
		session->file.fp = NULL;
		session->hpack_body_type = 2;

		client_push_msg(session, "\xEE\xEE\XEE", 3);
		///////////////////////////////
	}
	if (session->hpack_body_type == 2) { //其他数据读取
		rc = read(sock, &tmpbyte, 1);
		if (rc > 0) {

		} else {
			log_d("数据出错:rc=%d", rc);

			if (rc == 0)
				return -1;
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				return 0;
			return -1;
		}
	}
	return 0;
}

int client_push_msg(struct tcp_client *session, void *data, int dlen) {
	struct cpacket *pack = (struct cpacket *) malloc(sizeof(struct cpacket));
	if (!pack)
		return -1;

	memset(pack, 0, sizeof(struct cpacket));
	pack->data = (unsigned char *) malloc(dlen);
	memcpy(pack->data, data, dlen);
	pack->dlen = dlen;

	if (session->packet_head == NULL) {
		session->packet_head = pack;
		session->packet_last = pack;
	} else {
		session->packet_last->next = pack;
		session->packet_last = pack;
	}
	return 0;
}

int client_send_message(struct tcp_client *session) {
	int ret = 0;
	ret = send(session->sock, session->packet_cur->data + session->packet_cur->pos,
			session->packet_cur->dlen - session->packet_cur->pos, 0);
	if (ret > 0) {
		session->packet_cur->pos += ret;
		if (session->packet_cur->pos == session->packet_cur->dlen) {
			log_hex(session->packet_cur->data, session->packet_cur->dlen, "send message:");

			session->packet_head = session->packet_head->next;
			free(session->packet_cur->data);
			free(session->packet_cur);
			session->packet_cur = NULL;
		}
	} else {
		if (ret == 0) {
			//error
			return -1;
		}
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return 0;
		}
		//error
		return -1;
	}
	return 0;
}

void client_disconnect(struct tcp_client *session) {
	struct cpacket *pack, *next;
	if (session->sock != -1) {
		shutdown(session->sock, SHUT_RDWR);
		close(session->sock);
		session->sock = -1;
	}
	if (session->file.fp)
		fclose(session->file.fp);
	session->file.fp = NULL;

	if (session->hpack._head_msg)
		free(session->hpack._head_msg);
	session->hpack._head_msg = NULL;

	pack = session->packet_head;
	while (pack) {
		if (pack->data)
			free(pack->data);
		next = pack->next;
		free(pack);
		pack = next;
	}

	list_del(&session->list);
	free(session);
}

int main(int argc, char **argv) {
	struct tcp_client *session = NULL;
	int epfd = epoll_create1(0);
	int fd_listen = socket_create(SOCK_STREAM, "", 1883);
	int fd_udp = socket_create(SOCK_DGRAM, "", 1884);

	struct epoll_event events[100];
	int nfds;
	int i = 0;

	struct list_head list_clients;
	int events_count = 2;
	int ret = 0;

	mem_init();

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
		nfds = epoll_wait(epfd, events, events_count, 1000 * 10);

		for (i = 0; i < nfds; ++i) {
			if (events[i].events & EPOLLIN) {		//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
				//UDP
				if (events[i].data.ptr == &fd_udp) {

					switch (_process_recvfrom(fd_udp)) {
					case 0:
						break;
					case 1: {
						int count = 0;
						struct list_head *pos, *n;
						list_for_each_safe(pos, n, &list_clients)
						{
							count++;
						}
						log_d("client-count:%d, events_count: %d", count, events_count - 2);
					}
						break;
					case 2:
						log_d("mem info>>>");
						mem_debug();
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
					ret = client_read_message_headpack(session);

					if (ret) {
						log_d("read message err.");
						epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
						client_disconnect(session);
						session = NULL;
						events_count--;
						continue;
					}

					if (session->packet_head != NULL) {
						epoll_ctl2(session->epfd, EPOLL_CTL_MOD, session->sock,
						EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLRDHUP, session);
					}
				}
			}

			if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				if (events[i].data.ptr != &fd_listen && events[i].data.ptr != &fd_udp) {
					session = events[i].data.ptr;

					if (session->packet_cur == NULL) {
						session->packet_cur = session->packet_head;
					}

					if (session->packet_cur) {
						if (client_send_message(session) == 0) {
							if (session->packet_head == NULL) {
								epoll_ctl2(session->epfd, EPOLL_CTL_MOD, session->sock,
								EPOLLIN | EPOLLERR | EPOLLRDHUP, session);
							}
						} else {
							epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
							client_disconnect(session);
							events_count--;
							session = NULL;
						}
					}
				}
			}

			if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
				if (events[i].data.ptr != &fd_listen && events[i].data.ptr != &fd_udp) {
					session = events[i].data.ptr;

					epoll_ctl2(epfd, EPOLL_CTL_DEL, session->sock, 0, 0);
					client_disconnect(session);
					session = NULL;
					events_count--;
				}
				log_d("EPOLLERR");
			}
			//客户端有事件到达
			if (events[i].events & EPOLLHUP) {
				log_d("EPOLLHUP");
			}
		}
	}

	return 0;
}
