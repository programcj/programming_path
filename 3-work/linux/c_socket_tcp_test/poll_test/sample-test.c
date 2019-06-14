/*
 ============================================================================
 Name        : test_time.c
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>

#include "lqueue.h"
#include "proto_service.h"

#if 0
struct list_item {
	struct list_head list_node;
	int v;
};

struct list_queue queue;

long time_long() {
#if 1
	struct timespec tv;
	long v;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	v = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
#else
	struct timeval tv;
	long v;
	gettimeofday(&tv, NULL);
	v = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	return v;
}

void log(const char *fun, int line, const char *format, ...) {
	va_list v;
	va_start(v, format);
	printf("[%ld] %s,%d:", time_long(), fun, line);
	vfprintf(stdout, format, v);
	va_end(v);
}
#define printf(...)	log(__FUNCTION__, __LINE__, ##__VA_ARGS__)

void *_run(void *args) {
	struct list_head *list_item;
	struct list_item *node = NULL;
	long tm[5];

	sleep(1);
	tm[0] = time_long();
	while (1) {
		tm[1] = time_long();
		list_queue_wait(&queue, 1);
		tm[2] = time_long();

		list_item = list_queue_pop(&queue);
		if (list_item == NULL)
		continue;
		tm[3] = time_long();

		node = list_entry(list_item, struct list_item, list_node);

		if (node->v == 9999)
		break;
		free(node);
	}
	printf("start:%ld, wait:%ld, pop:%ld item.v=%d\n", tm[3] - tm[0],
			tm[2] - tm[1], tm[3] - tm[2], node->v);
	return NULL;
}

struct list_item *item_new() {
	struct list_item *item = (struct list_item*) malloc(
			sizeof(struct list_item));
	item->v = rand() % 10;
	return item;
}
#endif

#include <malloc.h>

int rwhandle(struct proto_service_context *context,
		struct proto_session *session, int event) {
	char buff[100];
	int ret = 0;
	int fd = session->fd;

	memset(buff, 0, sizeof(buff));

	switch (event) {
	case PS_EVENT_SESSION_CREATE:
		printf("PS_EVENT_SESSION_CREATE fd=%d,%s\n", fd, buff);
		break;
	case PS_EVENT_SESSION_DESTORY:
		printf("PS_EVENT_SESSION_DESTORY fd=%d,%s\n", fd, buff);
		break;
	case PS_EVENT_ACCEPT:
		printf("PS_EVENT_ACCEPT, fd=%d\n", fd);
		break;
	case PS_EVENT_RECV: {
		ret = recv(fd, buff, sizeof(buff), 0);
		proto_service_session_timeout_start(session, 3);
		printf("PS_EVENT_RECV fd=%d,%s\n", fd, buff);
		if (ret == 0)
			return -1;
		proto_service_on_write(context, session, 1);
	}
		break;
	case PS_EVENT_WRITE:

		proto_service_session_timeout_stop(session);
		strcpy(buff, "hello my is epoll server!\n");
		ret = send(fd, buff, strlen(buff), 0);
		printf("PS_EVENT_WRITE ret=%d fd=%d,%s\n", ret, fd, buff);
		//proto_service_on_write(context, session, 0);

		if (proto_service_session_is_choke(session)) {
			proto_service_session_timeout_start(session, 3);
		}

		if (ret < strlen(buff)) {
			printf("ret < strlen(buff)\n");
		}

		if (ret == 0)
			return -1;

		break;

	case PS_EVENT_CLOSE:
		printf("PS_EVENT_CLOSE fd=%d,%s\n", fd, buff);
		break;

	case PS_EVENT_TIMEROUT:
		printf("PS_EVENT_TIMEROUT fd=%d\n", fd);

		ret = send(fd, "123\n", 4, 0);

		printf("ret=%d , is choke:%d\n", ret,
				proto_service_session_is_choke(session));
		proto_service_on_write(context, session, 1);
		break;
	}
	return 0;
}

#include "lqueue.h"

int main(void) {
	struct proto_service_context service;
	srand((unsigned int) getpid());

	lqueue_test();
	exit(1);

	//mallopt(M_MMAP_MAX, 0); // 禁止malloc调用mmap分配内存
	//mallopt(M_TRIM_THRESHOLD, -1); // 禁止内存紧缩
	//mallopt(M_MMAP_THRESHOLD, 1024 * 1024);
	memset(&service, 0, sizeof(service));

	service.backfun_rwhandle = rwhandle;
	proto_service_init(&service);
	proto_service_add_listen(&service, 1883);
	proto_service_add_listen(&service, 1884);
	proto_service_add_listen(&service, 1885);

	printf("loop start...\n");
	proto_service_loop(&service);
	//需要把这些fd手动关闭
	proto_service_destory(&service);

	printf("----------\n");
	return EXIT_SUCCESS;
}
