/*
 * epollex.h
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

#ifndef EPOLLEX_H_
#define EPOLLEX_H_

#include <sys/epoll.h>

#ifdef __cplusplus
extern "C" {
#endif

struct epollex {
	int epfd; //epoll 句柄
	int event_count; //event个数
//int *fds;
};

int epollex_create1(struct epollex *ep, int flag);

int epollex_ctl_add(struct epollex *ep, int fd, struct epoll_event *ev);

int epollex_ctl_mod(struct epollex *ep, int fd, struct epoll_event *ev);

int epollex_ctl_del(struct epollex *ep, int fd);

int epollex_ctl_get(struct epollex *ep, int fd, struct epoll_event *ev);

//add的总个数
int epollex_count(struct epollex *ep);

int epollex_wait(struct epollex *ep, struct epoll_event *__events,
		int __maxevents, int __timeout);

#ifdef __cplusplus
}
#endif

#endif /* EPOLLEX_H_ */
