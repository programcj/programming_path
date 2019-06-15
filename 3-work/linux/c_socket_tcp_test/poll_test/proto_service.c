/*
 * proto_service.c
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "proto_service.h"

static inline int socket_set(int fd, int leve, int opt, int v) {
	return setsockopt(fd, leve, opt, (char*) &v, sizeof v);
}

static inline int socket_nonblock(int fd, int flag) {
	int opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1) {
		return -1;
	}
	if (flag)
		opt |= O_NONBLOCK;
	else
		opt &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, opt) == -1) {
		return -1;
	}
	return 0;
}

void *proto_malloc(size_t size) {
	return malloc(size);
}
void proto_free(void *ptr) {
	free(ptr);
}

void *proto_realloc(void *ptr, size_t size) {
	return realloc(ptr, size);
}
void *proto_calloc(size_t c, size_t s) {
	return calloc(c, s);
}

#include <time.h>

//1s=1000ms
//1ms=1000us
//1us=1000ns
//nsec = 纳秒
uint64_t proto_service_monotonic_timestamp_ms() {
#if 1
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv); //system power start
	return (uint64_t) tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 +tv.tv_usec/1000; //1532458552
#endif
}

void proto_service_init(struct proto_service_context *pser) {
	pser->epollfd = epoll_create1(0);
	//pipe(pser->pipefd);
	pser->session_list_head.next = &pser->session_list_head;
	pser->session_list_head.prev = &pser->session_list_head;

	socketpair(AF_UNIX, SOCK_STREAM, 0, pser->sfd);
}

static int _proto_service_close_session(struct proto_service_context *context,
		struct proto_session *s);

void proto_service_destory(struct proto_service_context *pser) {
	int i;
	int epollfd = pser->epollfd;
	struct proto_session *sp = NULL, *sn = NULL;

	for (sp = pser->session_list_head.next, sn = sp->next;
			sp != &pser->session_list_head; sp = sn, sn = sp->next) {
		_proto_service_close_session(pser, sp);
	}

	epoll_ctl(epollfd, EPOLL_CTL_DEL, pser->sfd[0], NULL);

	for (i = 0; i < pser->listen_fdsize; i++) {
		epoll_ctl(epollfd, EPOLL_CTL_DEL, pser->listen_fds[i], NULL);

		if (pser->listen_fds[i] != -1) {
			close(pser->listen_fds[i]);
			pser->listen_fds[i] = -1;
		}
	}
	pser->listen_fdsize = 0;

	close(pser->epollfd);
	close(pser->sfd[0]);
	close(pser->sfd[1]);
}

int proto_service_on_write(struct proto_service_context *context,
		struct proto_session *session, int flag) {
	struct epoll_event ev;
	ev.data.fd = session->fd;
	ev.data.ptr = session;
	ev.events = EPOLLIN;
	if (flag) {
		ev.events |= EPOLLOUT;
	}
	return epoll_ctl(context->epollfd, EPOLL_CTL_MOD, session->fd, &ev);
}

int proto_service_session_timeout_start(struct proto_session *session,
		int timeout_s) {
	session->interval_start = proto_service_monotonic_timestamp_ms();
	session->interval = timeout_s * 1000;
	printf(".....[%d] %d\n", session->fd, session->interval_start);
	return 0;
}

int proto_service_session_timeout_stop(struct proto_session *session) {
	session->interval_start = 0;
	session->interval = 0;
	return 0;
}

int proto_service_session_is_choke(struct proto_session *session) {
	struct pollfd pofd;
	memset(&pofd, 0, sizeof(pofd));
	pofd.fd = session->fd;
	pofd.events = POLLOUT;

	if (poll(&pofd, 1, 0) != 1)
		return 1;

	return pofd.revents & POLLOUT ? 0 : 1;
}

void proto_service_signal_quit_wait(struct proto_service_context *context) {
	send(context->sfd[1], "W", 1, 0);
}

int proto_service_add_listen(struct proto_service_context *context, int port) {
	int *pfd = NULL;
	int fd = -1;
	struct sockaddr_in saddr;
	char *local_addr = "0.0.0.0";
	int ret = 0;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;

	bzero(&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	inet_pton(AF_INET, local_addr, &(saddr.sin_addr)); //htons(portnumber);

	ret = socket_set(fd, SOL_SOCKET, SO_REUSEPORT, 1);
	if (ret == -1)
		goto __err;

	ret = socket_set(fd, SOL_SOCKET, SO_REUSEPORT, 1);
	if (ret == -1)
		goto __err;

	ret = bind(fd, (struct sockaddr*) &saddr, sizeof(saddr));
	if (ret == -1)
		goto __err;

	ret = listen(fd, 50);
	if (ret == -1)
		goto __err;

	context->listen_fdsize++;
	if (context->listen_fds) {
		pfd = (int*) proto_realloc(context->listen_fds,
				context->listen_fdsize * sizeof(int));
		if (!pfd)
			goto __err;
		context->listen_fds = pfd;
	} else {
		context->listen_fds = (int*) proto_malloc(
				context->listen_fdsize * sizeof(int));
		if (!context->listen_fds)
			goto __err;
	}
	context->listen_fds[context->listen_fdsize - 1] = fd;

	{
		struct epoll_event ev;
		ev.data.fd = fd;
		ev.events = EPOLLIN;
		epoll_ctl(context->epollfd, EPOLL_CTL_ADD, fd, &ev);
	}
	return 0;

	__err: if (fd != -1)
		close(fd);
	return -1;
}

struct proto_session *proto_serssion_new() {
	struct proto_session *session = proto_malloc(sizeof(struct proto_session));
	if (session)
		memset(session, 0, sizeof(struct proto_session));
	return session;
}

void proto_service_append_session(struct proto_service_context *context,
		struct proto_session *s) {
	s->prev = context->session_list_head.prev;
	s->next = &context->session_list_head;
	context->session_list_head.prev->next = s;
	context->session_list_head.prev = s;
	context->session_count++;
}

static int _proto_service_close_session(struct proto_service_context *context,
		struct proto_session *s) {
	int ret;

	context->session_count--;
	context->backfun_rwhandle(context, s, PS_EVENT_SESSION_DESTORY);

	if (s->fd != -1) {
		s->next->prev = s->prev;
		s->prev->next = s->next;
		close(s->fd);
	}
	ret = epoll_ctl(context->epollfd, EPOLL_CTL_DEL, s->fd, NULL);
	s->fd = -1;
	//context->session_count--;
	//context->backfun_rwhandle(context, s, PS_EVENT_SESSION_DESTORY);
	proto_free(s);
	return ret;
}

int proto_service_session_noinbkfun_close(struct proto_service_context *context,
		struct proto_session *s) {
	return _proto_service_close_session(context, s);
}

int proto_service_loop(struct proto_service_context *context) {
	int epollfd;
	int i;
	int fdcount = 0;
	int epollsize1 = 0;
	int ret;
	long time_interval = 0;

	if (!context->listen_fdsize)
		return -1;

	epollfd = context->epollfd;

	if (!context->epoll_events) {
		context->epoll_events_count = context->listen_fdsize;
		context->epoll_events = malloc(
				context->epoll_events_count * sizeof(struct epoll_event));
	}

	struct proto_session *session = NULL, *sn = NULL, *sp = NULL;
	int interval_min = 100;

	{ //寻找最小定时器时间
		time_interval = proto_service_monotonic_timestamp_ms();

		interval_min = 100;
		for (sp = context->session_list_head.next, sn = sp->next;
				sp != &context->session_list_head; sp = sn, sn = sp->next) {
			if (sp->interval_start) {
				if (interval_min > sp->interval)
					interval_min = sp->interval;
			}
		}
	}

	{ //
		epollsize1 = context->listen_fdsize + context->session_count;

		if (context->epoll_events_count != epollsize1) {
			void *eallptr = realloc(context->epoll_events,
					epollsize1 * sizeof(struct epoll_event));
			if (eallptr) {
				context->epoll_events = eallptr;
				context->epoll_events_count = epollsize1;
			}
		}
		memset(context->epoll_events, 0, context->epoll_events_count);
	}

	//毫秒延时 1s=1000ms
	fdcount = epoll_wait(epollfd, context->epoll_events,
			context->epoll_events_count, interval_min);

	if (fdcount == -1) {
		perror("epoll wait...\n");
		if (errno == EINTR)
			return 0;
		return -1;
	}

	time_interval = proto_service_monotonic_timestamp_ms();

	//寻找定时器
	{
		for (sp = context->session_list_head.next, sn = sp->next;
				sp != &context->session_list_head; sp = sn, sn = sp->next) {

			if (sp->interval_start) {
				if (time_interval > sp->interval_start + sp->interval) {
					sp->interval_start = 0;
					sp->interval = 0;
					ret = context->backfun_rwhandle(context, sp,
							PS_EVENT_TIMEROUT);

					if (ret == -1)
						_proto_service_close_session(context, sp);
				}
			}
		}
	}

	if (fdcount == 0) {
		return 0;
	}

	//读写数据处理
	for (i = 0; i < fdcount; i++) {
		int listeni;

		//监听处理 listen accept handle
		for (listeni = 0; listeni < context->listen_fdsize; listeni++) {
			if (context->epoll_events[i].data.fd
					== context->listen_fds[listeni]) {
				if (context->epoll_events[i].events & (EPOLLIN | EPOLLPRI)) {
					struct epoll_event ev;
					{
						struct sockaddr_in caddr;

						socklen_t caddrlen = sizeof(caddr);
						ev.data.fd = accept(context->epoll_events[i].data.fd,
								(struct sockaddr*) &caddr, &caddrlen);

						if (ev.data.fd == -1)
							continue;

						session = proto_serssion_new();
						if (!session) {
							close(ev.data.fd);
							continue;
						}
						session->peerport = ntohs(caddr.sin_port);
					}

					session->fd = ev.data.fd;
					socket_nonblock(ev.data.fd, 1);

					ev.events = EPOLLIN;
					ev.data.ptr = session;

					proto_service_append_session(context, session);
					ret = context->backfun_rwhandle(context, session,
							PS_EVENT_SESSION_CREATE);

					ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, session->fd, &ev);
					if (ret != 0) {
						_proto_service_close_session(context, session);
						break;
					}

					ret = context->backfun_rwhandle(context, session,
							PS_EVENT_ACCEPT);

					if (ret != 0) {
						_proto_service_close_session(context, session);
						break;
					}
				}
				break;
			}
		}

		if (listeni != context->listen_fdsize)
			continue;

		//会话处理 event handle
		session = (struct proto_session *) context->epoll_events[i].data.ptr;
		int fd = session->fd;
		uint32_t events = context->epoll_events[i].events;

		if (events & EPOLLOUT) {
			ret = context->backfun_rwhandle(context, session, PS_EVENT_WRITE); //events[i].data.fd //need write data
			if (-1 == ret) {
				_proto_service_close_session(context, session);
				continue;
			}
		}

		if (events & EPOLLIN) {
			//events[i].data.fd  //need read data
			if (fd == context->sfd[0]) {
				memset(&context->sfddata, 0, sizeof(context->sfddata));
				printf("signale....\n");
				recv(fd, &context->sfddata, 1, 0);
				if (context->sfddata[0] == 'q')
					return -1;
			} else {
				ret = context->backfun_rwhandle(context, session,
						PS_EVENT_RECV);
				if (-1 == ret) {
					_proto_service_close_session(context, session);
					continue;
				}
			}
		}

		if (events & (EPOLLERR | EPOLLHUP)) {
			ret = context->backfun_rwhandle(context, session, PS_EVENT_CLOSE);
			_proto_service_close_session(context, session);
		}
		///////////////////
	}
	//end epoll_wait
	return 0;
}

void proto_service_quit(struct proto_service_context *context) {
	send(context->sfd[1], "q", 1, 0);
}
