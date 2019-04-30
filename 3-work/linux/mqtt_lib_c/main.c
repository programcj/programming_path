#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/tcp.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "libemqtt.h"
#include "clog.h"

#define printf	log_d
#include <pthread.h>

static int _socket_nonblock(int sock) {
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

#include "poll.h"

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
	int sock;
	char ip[50];
	unsigned short port;
	char clientid[50];

	struct mqtt_packet inpacket;
	struct mqtt_packet *outpacket;
	struct mqtt_packet *outpacket_last;
	struct mqtt_packet *curr_out_packet;

	int heart_time;
	mqtt_broker_handle_t broker;

	pthread_t pt;
	int thread_run_flag;
};

typedef struct mqtt_session * CMQTT;

int mqtt_packet_read(int sock, struct mqtt_packet *pack,
		void (*process_pack_cb)(struct mqtt_packet *pack, void *context), void *context) {
	unsigned char tmpbyte;
	int rc = 0;

	if (!pack->command) {
		rc = read(sock, &tmpbyte, 1);
		if (rc == 1) {
			pack->command = tmpbyte;
			pack->data[pack->pos++] = tmpbyte;
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
				pack->data[pack->pos++] = tmpbyte;
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
	if (process_pack_cb)
		process_pack_cb(pack, context);
	return 1;
}

const char *MQTT_TYPE_STRING[] = { ///
		"CONNECT",  ///
				"CONNACK ", ///
				"PUBLISH ", ///
				"PUBACK   ", ///
				"PUBREC", ///
				"PUBREL", ///
				"PUBCOMP ", ///
				"SUBSCRIBE", ///
				"SUBACK", ///
				"UNSUBSCRIBE", ///
				"UNSUBACK", ///
				"PINGREQ", ///
				"PINGRESP", ///
				"DISCONNECT"  ///
		};

void _mqtt_handle_pack(struct mqtt_packet *pack, void *context) {
	int type = MQTTParseMessageType(pack->data);
	if (((type >> 4) - 1) < 14) {
		log_d("%s \n", MQTT_TYPE_STRING[((type >> 4) - 1)]);
	}
	pack->pos = 0;
	pack->command = 0;
	pack->rem_byte_size = 0;
	pack->rem_length = 0;
	pack->rem_need_length = 0;
}

int mqtt_session_push_packet(void* socket_info, const void* buff, unsigned int count) {
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
	return 0;
}

int test() {
	int sock;
	int flag = 1;
	unsigned short port = 1883;
	const char *hostname = "192.168.205.131";
	int alive = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)) < 0) {
		close(sock);
		return -1;
	}
	struct sockaddr_in socket_address;
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.s_addr = inet_addr(hostname);
	if ((connect(sock, (struct sockaddr*) &socket_address, sizeof(socket_address))) < 0) {
		close(sock);
		return -2;
	}

	if (_socket_nonblock(sock) != 0) {
		close(sock);
		return -3;
	}
	log_d("connect success");

	struct pollfd pollfds;
	int fdcount = 0;
	int ret = 0;

	struct mqtt_session session;
	memset(&session, 0, sizeof(session));

	mqtt_init(&session.broker, "client-id");

	session.inpacket.data = malloc(20480);
	session.inpacket.dlen = 20480;

	session.sock = sock;
	session.broker.socket_info = &session;
	session.broker.send = mqtt_session_push_packet;
	alive = 30; //seconds
	mqtt_set_alive(&session.broker, alive);

	mqtt_connect(&session.broker);

	int alive_time = time(NULL);
	//mqtt_subscribe(&broker, "public/test/topic", &msg_id);
	while (1) {
		pollfds.fd = sock;
		pollfds.events = POLLIN;

		if (time(NULL) - alive_time > 30) {
			alive_time = time(NULL);
			log_d("ping....");
			mqtt_ping(&session.broker);
		}

		if (session.outpacket != NULL)
			pollfds.events |= POLLOUT;

		fdcount = poll(&pollfds, 1, 100);
		if (fdcount == -1) {
			log_d("poll err........");
		}

		if (pollfds.revents & POLLOUT) {
			if (session.curr_out_packet == NULL) {
				session.curr_out_packet = session.outpacket;
			}

			if (session.curr_out_packet) {

				ret = send(session.sock,
						session.curr_out_packet->data + session.curr_out_packet->pos,
						session.curr_out_packet->dlen - session.curr_out_packet->pos, 0);
				if (ret > 0) {
					session.curr_out_packet->pos += ret;
				} else {
					if (errno == EWOULDBLOCK || errno == EAGAIN) {
						continue;
					}
				}

				if (session.curr_out_packet->pos >= session.curr_out_packet->dlen) {
					session.outpacket = session.outpacket->next; //

					free(session.curr_out_packet->data);
					free(session.curr_out_packet);
					session.curr_out_packet = NULL;
				}
			}
		}
		if (pollfds.revents & POLLIN) {
			ret = mqtt_packet_read(sock, &session.inpacket, _mqtt_handle_pack, NULL);
			if (ret == 1) {
				//handle
				alive_time = time(NULL);

				log_d("收到数据:");
				session.inpacket.pos = 0;
				session.inpacket.command = 0;
				session.inpacket.rem_byte_size = 0;
				session.inpacket.rem_length = 0;
				session.inpacket.rem_need_length = 0;
			}
			if (ret < 0) {
				log_d("read err <0, close");
				break;
			}
		}
	}
	close(sock);
	free(session.inpacket.data);
	exit(-1);
}

int cmqtt_init(struct mqtt_session *session, const char *ip, unsigned short port,
		const char *clientid) {
	memset(session, 0, sizeof(struct mqtt_session));
	strcpy(session->ip, ip);
	strcpy(session->clientid, clientid);
	session->port = port;
	session->sock = -1;

	session->inpacket.data = (unsigned char *) malloc(20480);
	session->inpacket.dlen = 20480;
	return 0;
}

void cmqtt_destory(struct mqtt_session *session) {
	struct mqtt_packet *pack = NULL;
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
}

int cmqtt_connect(struct mqtt_session *session) {
	int sock;
	int flag = 1;
	int alive = 0;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)) < 0) {
		close(sock);
		return -1;
	}
	struct sockaddr_in socket_address;
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(session->port);
	socket_address.sin_addr.s_addr = inet_addr(session->ip);
	if ((connect(sock, (struct sockaddr*) &socket_address, sizeof(socket_address))) < 0) {
		close(sock);
		return -2;
	}

	if (_socket_nonblock(sock) != 0) {
		close(sock);
		return -3;
	}

	log_d("connect success");
	mqtt_init(&session->broker, "client-id");

	session->sock = sock;
	session->broker.socket_info = session;
	session->broker.send = mqtt_session_push_packet;
	alive = 30; //seconds
	mqtt_set_alive(&session->broker, alive);

	mqtt_connect(&session->broker);
	session->heart_time = time(NULL);
	return 0;
}

int cmqtt_recv_packet_clear(struct mqtt_session *session) {
	session->inpacket.pos = 0;
	session->inpacket.command = 0;
	session->inpacket.rem_byte_size = 0;
	session->inpacket.rem_length = 0;
	session->inpacket.rem_need_length = 0;
	return 0;
}

int cmqtt_recv_packet(struct mqtt_session *session) {
	struct pollfd pollfds;
	int fdcount = 0;
	int ret = 0;

	pollfds.fd = session->sock;
	pollfds.events = POLLIN;

	if (time(NULL) - session->heart_time > 30) {
		mqtt_ping(&session->broker);
	}

	if (session->outpacket != NULL)
		pollfds.events |= POLLOUT;

	fdcount = poll(&pollfds, 1, 200);
	if (fdcount == -1) {
		log_d("poll err........");
		return -1;
	}

	if (pollfds.revents & POLLOUT) {
		if (session->curr_out_packet == NULL) {
			session->curr_out_packet = session->outpacket;
		}

		if (session->curr_out_packet) {
			ret = send(session->sock,
					session->curr_out_packet->data + session->curr_out_packet->pos,
					session->curr_out_packet->dlen - session->curr_out_packet->pos, 0);
			if (ret > 0) {
				session->curr_out_packet->pos += ret;
			} else {
				if (errno == EWOULDBLOCK || errno == EAGAIN) {
					return 0;
				}
			}

			if (session->curr_out_packet->pos >= session->curr_out_packet->dlen) {
				session->outpacket = session->outpacket->next; //

				free(session->curr_out_packet->data);
				free(session->curr_out_packet);
				session->curr_out_packet = NULL;
			}
		}
	}
	if (pollfds.revents & POLLIN) {
		ret = mqtt_packet_read(session->sock, &session->inpacket, _mqtt_handle_pack, NULL);
		if (ret == 1) {
			//handle
			session->heart_time = time(NULL);

			log_d("收到数据:");
//			session->inpacket.pos = 0;
//			session->inpacket.command = 0;
//			session->inpacket.rem_byte_size = 0;
//			session->inpacket.rem_length = 0;
//			session->inpacket.rem_need_length = 0;
			return 1;
		}
		if (ret < 0) {
			log_d("read err <0, close");
			return ret;
		}
		return ret;
	}
	return 0;
}

int main(int argc, char **argv) {
	struct mqtt_session session;
	int connect_flag = 0;
	cmqtt_init(&session, "192.168.205.131", 1883, "1111111");

	if (cmqtt_connect(&session) == 0) {
		cmqtt_recv_packet_clear(&session);
		while (1) {
			if (cmqtt_recv_packet(&session) == 1) {
				switch (connect_flag) {
				case 1:
					mqtt_publish(&session.broker, "hello/emqtt", "Example: QoS 0", 0);
					connect_flag = 2;
					break;
				case 2:

					break;
				case 3:
					break;
				}
				if (connect_flag == 0) {
					if (MQTTParseMessageType(session.inpacket.data) == MQTT_MSG_CONNACK) {
						if (session.inpacket.data[3] == 0x00) {
							connect_flag = 1;
							log_d("connect mqtt ack success");
						} else {
							//connect error
							break;
						}
					}
				}
				cmqtt_recv_packet_clear(&session);
			}
		}
	}
	cmqtt_destory(&session);
	return 0;
}

//int main(int argc, char **argv) {
//	uint8_t packet_buffer[20480];
//	int packet_length = 0;
//
//	test();
//
//	mqtt_init(&broker, "client-id");
//	//mqtt_init_auth(&broker, "quijote", "rocinante");
//	if (init_socket(&broker, "192.168.205.131", 1883, 30) < 0) {
//		printf("connect error");
//		return -1;
//	}
//
//	printf("connect ok");
//
//	mqtt_connect(&broker);
//
//	memset(packet_buffer, 0, sizeof(packet_buffer));
//	packet_length = read_packet(&broker, packet_buffer, sizeof(packet_buffer), 10);
//	if (packet_length < 0) {
//		fprintf(stderr, "Error(%d) on read packet!\n", packet_length);
//		return -1;
//	}
//
//	if (MQTTParseMessageType(packet_buffer) != MQTT_MSG_CONNACK) {
//		fprintf(stderr, "CONNACK expected!\n");
//		return -2;
//	}
//
//	if (packet_buffer[3] != 0x00) {
//		fprintf(stderr, "CONNACK failed!\n");
//		return -2;
//	}
//	printf("connect success");
//	log_hex(packet_buffer, packet_length + 2, "packet_buffer:");
//
//	//mqtt_publish(&broker, "hello/emqtt", "Example: QoS 0", 0);
//
//	{
//		unsigned char data[2000];
//		int dlen = sizeof(data);
//		memset(data, 'E', sizeof(data));
//		data[sizeof(data) - 1] = 'O';
//		mqtt_publish_data(&broker, "hello/emqtt", data, dlen, 0, 0, NULL);
//	} //PUBACK 报文是对 QoS 1 等级的 PUBLISH 报文的响应
//	sleep(1);
//	{
//		uint16_t msg_id, msg_id_rcv;
//		char buff[1000];
//		memset(buff, 'A', sizeof(buff));
//		buff[999] = 0;
//
//		log_d("发布开始");
//		mqtt_publish_with_qos(&broker, "hello/emqtt", buff, 0, 1, &msg_id);
//
//		memset(packet_buffer, 0, sizeof(packet_buffer));
//		packet_length = read_packet(&broker, packet_buffer, sizeof(packet_buffer), 10);
//		if (MQTTParseMessageType(packet_buffer) != MQTT_MSG_PUBACK) {
//			fprintf(stderr, "PUBACK expected!\n");
//			return -2;
//		}
//
//		msg_id_rcv = mqtt_parse_msg_id(packet_buffer);
//		if (msg_id != msg_id_rcv) {
//			fprintf(stderr, "%d message id was expected, but %d message id was found!\n", msg_id,
//					msg_id_rcv);
//			return -3;
//		}
//
//		log_d("msg_id_rcv=%d", msg_id_rcv);
//	}
//	{
//		uint16_t msg_id, msg_id_rcv;
//		log_d("订阅开始");
//		mqtt_subscribe(&broker, "public/test/topic", &msg_id);
//		packet_length = read_packet(&broker, packet_buffer, sizeof(packet_buffer), 10);
//		if (packet_length < 0) {
//			fprintf(stderr, "Error(%d) on read packet!\n", packet_length);
//			return -1;
//		}
//
//		if (MQTTParseMessageType(packet_buffer) != MQTT_MSG_SUBACK) {
//			fprintf(stderr, "SUBACK expected!\n");
//			return -2;
//		}
//
//		msg_id_rcv = mqtt_parse_msg_id(packet_buffer);
//		if (msg_id != msg_id_rcv) {
//			fprintf(stderr, "%d message id was expected, but %d message id was found!\n", msg_id,
//					msg_id_rcv);
//			return -3;
//		}
//	}
//
//	while (1) {
//		// <<<<<
//		packet_length = read_packet(&broker, packet_buffer, sizeof(packet_buffer), 10);
//		if (packet_length == -1) {
//			fprintf(stderr, "Error(%d) on read packet!\n", packet_length);
//			return -1;
//		}
//		if (packet_length >= 0) {
//			printf("Packet Header: 0x%x...packet_length=%d", packet_buffer[0], packet_length);
//			if (MQTTParseMessageType(packet_buffer) == MQTT_MSG_PUBLISH) {
//				uint8_t topic[255], msg[1000];
//				uint16_t len;
//
//				printf("QOS=%d", MQTTParseMessageQos(packet_buffer));
//
//				len = mqtt_parse_pub_topic(packet_buffer, topic);
//				topic[len] = '\0'; // for printf
//				len = mqtt_parse_publish_msg(packet_buffer, msg);
//				msg[len] = '\0'; // for printf
//				printf("%s %s\n", topic, msg);
//			}
//
//			if (MQTTParseMessageType(packet_buffer) == MQTT_MSG_PINGRESP) {
//				printf("MQTT_MSG_PINGRESP");
//			}
//		}
//		if (packet_length == -88) {
//			printf("read time out");
//			mqtt_ping(&broker); //need pingresp
//		}
//	}
//	mqtt_disconnect(&broker);
//
//	shutdown((int) (broker.socket_info), SHUT_RDWR);
//	close((int) (broker.socket_info));
//	return 0;
//}
