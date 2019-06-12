/*
 ============================================================================
 Name        : test-DeviceBindTcp.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include "BindClient.h"
#include "BindServer.h"
#include "clog.h"

#include "CJProto_server.h"

#define IP_SERVER "127.0.0.1"

void _connectionLost(void* context, char* cause) {
	log_d("close...");
}

int _messageArrived(void* context, int type, void* data, int dlen) {
	log_d("(dlen:%d)%s", dlen, data);
	return 0;
}

BindServer server;

void* _t_server(void* args) {
	//    BindClient client = NULL;
	//    char *buff = "1111";
	//
	//    BindClient_create(&client);
	//    BindClient_setCallbacks(client, NULL, _connectionLost, _messageArrived);
	//    if (BindClient_connect(client, IP_SERVER, 1883) == 0) {
	//        while (BindClient_isConnected(client)) {
	//            BindClient_send(client, 1, buff, strlen(buff));
	//            log_d("%d",client);
	//            sleep(1);
	//        }
	//    }
	//    BindClient_close(client);
	//    BindClient_destroy(client);
	//

	//BindServer_create(&server);
	log_d("start server...");
	//BindServer_bind(server, "127.0.0.1", 8000);
	//BindServer_loop(server);
	CJProto_server_loop();
	return NULL;
}

#define list_def(type) \
    type* prev;        \
    type* next;        \
    type* head

#define list_varhead(type, v) type v = { .prev = &v, .next = &v, .head = &v }
#define list_append(varhead, item)          \
    do {                                    \
        if ((item)->head != (varhead)) {    \
            (varhead)->prev->next = (item); \
            (item)->prev = (varhead)->prev; \
            (item)->next = (varhead);       \
            (varhead)->prev = (item);       \
            (item)->head = (varhead);       \
        }                                   \
    } while (0)

#define list_foreach_next(pos, n, varhead) \
    for (pos = (varhead)->next, n = pos->next; pos != (varhead); pos = n, n = n->next)

#define list_del(item)                 \
    do {                               \
        item->next->prev = item->prev; \
        item->prev->next = item->next; \
        item->head = NULL;             \
    } while (0)

struct list_test {
	list_def(struct list_test)
	;
	int v;
};

list_varhead(struct list_test, testhead);

void test() {
	struct list_test v;
	memset(&v, 0, sizeof(v));
	v.v = 10;
	list_append(&testhead, &v);

	struct list_test *p, *n;
	list_foreach_next(p, n, &testhead)
	{
		printf("%d\n", p->v);
	}
}

struct rtimer {
	list_def(struct rtimer)
	;

	long start;
	long interval; //usec

	void (*callback)(struct rtimer*, void*);
	void* context;
};

//static struct rtimer listhead = { .prev = &listhead, .next = &listhead };
static list_varhead(struct rtimer, listhead);

long rtimer_nowus() {
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv); //系统启动时间
	//1s=1000ms
	//1ms=1000us
	//1us=1000ns
	return tv.tv_sec * 1000 * 1000 + (tv.tv_nsec / 1000);
}

int rtimer_set(struct rtimer* rt, int usec, void (*fun)(struct rtimer*, void*),
		void* ctx) {
	struct rtimer* tmp = listhead.prev;
	//if (rt->head == &listhead)
	//     return -1;
	//rt->head = &listhead;

	rt->interval = usec;
	rt->start = rtimer_nowus();
	rt->callback = fun;
	rt->context = ctx;

	list_append(&listhead, rt);

	// tmp->next = rt;
	// rt->prev = tmp;
	// rt->next = &listhead;
	// listhead.prev = rt;
	return 0;
}

void rtimer_call() {
	struct rtimer *p, *n;
	//for (p = listhead.next, n = p->next; p != &listhead; p = n, n = n->next) {

	list_foreach_next(p, n, &listhead)
	{
		if (rtimer_nowus() - p->start > p->interval) {
			// p->prev->next = p->next;
			// p->next->prev = p->prev;
			// p->head = (void*)0;
			list_del(p); //do { p->next->prev=p->prev; p->prev->next=p->next;p->head=NULL; } while(0);

			p->callback(p, p->context);
		}
	}
}

#include "execinfo.h"
void bafun(struct rtimer* rt, void* arg) {
	void* arr[5];
	char** strings;
	int i = 0;
	int len = backtrace(arr, 5);
	strings = backtrace_symbols(arr, len);
	//for (i = 0; i < len; i++)
	//	printf("  [%02d] %s\n", i, strings[i]);
	free(strings);
	log_d("[%ld] %ld %s", rtimer_nowus(), rt->start, arg);
}

#include <sys/socket.h>

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
		proto_service_session_start_timeout(session, 3);
		printf("PS_EVENT_RECV fd=%d,%s\n", fd, buff);
		if (ret == 0)
			return -1;
		//proto_service_on_write(context, session, 1);
	}
		break;
	case PS_EVENT_WRITE:
		strcpy(buff, "hello my is epoll server!\n");
		ret = write(fd, buff, strlen(buff));
		printf("PS_EVENT_WRITE fd=%d,%s\n", fd, buff);
		proto_service_on_write(context, session, 0);
		break;
	case PS_EVENT_CLOSE:
		printf("PS_EVENT_CLOSE fd=%d,%s\n", fd, buff);
		break;
	case PS_EVENT_TIMEROUT:
		printf("PS_EVENT_TIMEROUT fd=%d\n", fd);
		proto_service_on_write(context, session, 1);
		break;
	}
	return 0;
}

struct proto_service_context service;

void *_RunServiceExit(void *args) {
	sleep(10);
	service.run_loop = 0;
	printf("_RunServiceExit...");
	proto_service_signal_quit_wait(&service);
	return NULL;
}

int main(int argc, char** argv) {
	memset(&service, 0, sizeof(struct proto_service_context));
	pthread_t pt;
	//pthread_create(&pt, NULL, _RunServiceExit, NULL);

	service.backfun_rwhandle = rwhandle;
	proto_service_init(&service);
	proto_service_add_listen(&service, 1883);
	proto_service_add_listen(&service, 1884);
	proto_service_add_listen(&service, 1885);

	proto_service_loop(&service);
	//需要把这些fd手动关闭
	proto_service_destory(&service);

	printf("close.....\n");
	exit(1);

#if 1
	struct rtimer rtime1, rtime2, rtime3;
	test();

	rtimer_set(&rtime1, 100, bafun, "rtime1");
	rtimer_set(&rtime2, 300, bafun, "rtime2");
	rtimer_set(&rtime3, 1000 * 1000 * 5, bafun, "rtime5");
	rtimer_set(&rtime1, 100, bafun, "rtime1");

	while (1) {
		rtimer_call();
	}
#else
	pthread_t pt;
	char* buff = "111111111111112341231231232132132131232132131231231";

	//    BindServer_loop(NULL);

	// _t_server(NULL);
	log_d("%02X", strlen(buff));

	BindClient client = NULL;

	pthread_create(&pt, NULL, _t_server, NULL);
	//pthread_create(&pt, NULL, _t_server, NULL);
	//pthread_create(&pt, NULL, _t_server, NULL);

	sleep(1);

	BindClient_create(&client);
	BindClient_setCallbacks(client, NULL, _connectionLost, _messageArrived);
	log_d("connect ....");
	if (BindClient_connect(client, IP_SERVER, 1883) == 0) {
		while (BindClient_isConnected(client)) {
			BindClient_send(client, 1, buff, strlen(buff));
			log_d("%d", client);
			sleep(1);
		}
	}
	BindClient_close(client);
	BindClient_destroy(client);

	sleep(1);
	//BindServer_close(server);

#endif
	return EXIT_SUCCESS;
}
