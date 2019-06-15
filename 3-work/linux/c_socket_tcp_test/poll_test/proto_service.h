/*
 * proto_service.h
 *
 *  Created on: 2019年6月14日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */

#ifndef PROTO_SERVICE_H_
#define PROTO_SERVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

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

	unsigned short peerport;

	void *user;
	uint32_t interval_start; //开始时间 ms
	long interval; // ms
};

struct proto_service_context {
	int *listen_fds;
	int listen_fdsize;
	int listen_port;
	int run_loop;
	int epollfd;
	int sfd[2]; //0 :read, 1:write
	int sfddata;

	void *user; //bind data

	int (*backfun_rwhandle)(struct proto_service_context *context,
			struct proto_session *session, int event);
	struct proto_session session_list_head;
	int session_count;
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

int proto_service_session_timeout_start(struct proto_session *session,
		int timeout_s);

int proto_service_session_timeout_stop(struct proto_session *session);

int proto_service_session_is_choke(struct proto_session *session);

void proto_service_signal_quit_wait(struct proto_service_context *context);

void proto_service_loop(struct proto_service_context *context);

uint64_t proto_service_monotonic_timestamp_ms();

#ifdef __cplusplus
}
#endif

#endif /* PROTO_SERVICE_H_ */
