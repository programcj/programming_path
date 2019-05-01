/*
 ============================================================================
 Name        : test_server_client.c
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "sockets.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#pragma pack(1)

struct type_file {
	unsigned char type;
	char file_name[30];
	unsigned long file_size;
};

struct packet {
	unsigned char _pack_head; //数据包头
	unsigned short _msg_len; //消息头长度
};

#pragma pack()

int main(int argc, char **argv) {
	int sock = 0;
	socklen_t addr_length;
	struct sockaddr_in addr;
	int ret = 0, slen = 0;
	unsigned char buff[20480];
	const char *file_path = "";  //"/home/cj/timg.jpeg"
	//const char *ip="192.168.1.124";
	const char *ip = "192.168.1.205";
	int i = 0;

	if (argc > 1) {
		file_path = argv[1];
	} else {
		//	file_path = "/home/cj/PDF189-20120911104202-SuanFaDaoLunYuanShuDiErBan.pdf";
		file_path = "/home/cj/timg.jpeg";
	}

	FILE *fp = NULL;

	signal(SIGPIPE, SIG_IGN);

	fp = fopen(file_path, "rb");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in_init(&addr, AF_INET, ip, 1883);
	addr_length = sizeof(struct sockaddr_in);
	ret = connect(sock, (struct sockaddr*) &addr, addr_length);

	unsigned char packet_head[sizeof(struct packet) + sizeof(struct type_file)];
	memset(packet_head, 0, sizeof(packet_head));

	struct packet *pack = (struct packet *) packet_head;
	struct type_file *type = (struct type_file *) (packet_head + sizeof(struct packet));

	type->type = 1;
	strcpy(type->file_name, file_path);
	{
		struct stat statbuff;
		stat(file_path, &statbuff);
		type->file_size = statbuff.st_size;
	}
	pack->_msg_len = sizeof(struct type_file);
	pack->_msg_len = ntohs(pack->_msg_len);

	printf("pack->_msg_len=%04X,  file size:%ld \n", pack->_msg_len, type->file_size);

	socket_send(sock, packet_head, sizeof(packet_head));
	sleep(2);
	while (1) {
		ret = fread(buff, 1, sizeof(buff), fp);
		if (ret > 0) {
			slen = socket_send(sock, buff, ret);
			if (slen <= 0)
				break;
			printf("sock send:%d (ret=%d)\n", slen, ret);
		} else {
			printf(">>>>>>>>>>>>>>>> ret=%d\n", ret);
			break;
		}
	}
	printf("ret=%d\n", ret);
	ret=0;
	ret = socket_recv(sock, buff, sizeof(buff));
	if (ret > 0) {
		for (i = 0; i < ret; i++) {
			printf("%02X ", buff[i]);
		}
		printf("\n");
	}

	shutdown(sock, SHUT_RDWR);
	close(sock);

	fclose(fp);

	return EXIT_SUCCESS;
}
