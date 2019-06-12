//
//  CJProto_server.h
//  CJTCPClient
//
//  Created by   CC on 2017/12/2.
//  Copyright © 2017年 CC. All rights reserved.
//

#ifndef CJProto_server_h
#define CJProto_server_h

#include "BindServer.h"

void CJProto_server_loop();

enum proto_service_event {
	PS_EVENT_SESSION_CREATE,
	PS_EVENT_SESSION_DESTORY,
	PS_EVENT_ACCEPT,
	PS_EVENT_CLOSE,
	PS_EVENT_RECV,
	PS_EVENT_WRITE,
	PS_EVENT_TIMEROUT,
};

struct proto_service_context;

struct proto_session {
	struct proto_session *next;
	struct proto_session *prev;
	struct proto_service_context *context;
	int fd;

	long interval_start; //开始时间 ms
	long interval; // ms
};

struct proto_service_context {
	int *listen_fds;
	int listen_fdsize;
	int listen_port;
	int run_loop;
	int epollfd;
	int pipefd[2]; //0 :read, 1:write
	int pipedata;

	void *user; //bind data

	int (*backfun_rwhandle)(struct proto_service_context *context,
			struct proto_session *session, int event);
	struct proto_session session_list_head;
};

void *proto_malloc(size_t size);
void proto_free(void *ptr);
void *proto_realloc(void *ptr, size_t size);
void *proto_calloc(size_t c, size_t s);

void proto_service_init(struct proto_service_context *context);
void proto_service_destory(struct proto_service_context *context);

int proto_service_add_listen(struct proto_service_context *context, int port);

int proto_service_on_write(struct proto_service_context *context,
		struct proto_session *session, int flag);

int proto_service_session_start_timeout(struct proto_session *session,
		int timeout_s);

void proto_service_signal_quit_wait(struct proto_service_context *context);

void proto_service_loop(struct proto_service_context *context);

#endif /* CJProto_server_h */
