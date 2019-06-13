/*
 * socketpair_test.c
 *
 *  Created on: 2019年6月13日
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int main(int argc, char **argv) {
	int fds[2];
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

	char *buff = "my is fds[0]\n";
	send(fds[0], buff, strlen(buff), 0);
	buff = "my is fds[1]\n";
	send(fds[1], buff, strlen(buff), 0);

	char array[100];
	memset(array, 0, sizeof(array));
	recv(fds[0], array, sizeof(array), 0);
	printf("fds[0] recv:%s", array);

	memset(array, 0, sizeof(array));
	recv(fds[1], array, sizeof(array), 0);
	printf("fds[1] recv:%s", array);

	return 0;
}

