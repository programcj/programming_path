/*
 * epollex.c
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
#include <sys/epoll.h>
#include "epollex.h"

int epollex_create1(struct epollex *ep, int flag) {
	ep->epfd = epoll_create1(flag);
	ep->event_count = 0;
	if (ep->epfd != -1)
		return 0;
	return -1;
}

int epollex_ctl_add(struct epollex *ep, int fd, struct epoll_event *ev) {
	int ret;
	ret = epoll_ctl(ep->epfd, EPOLL_CTL_ADD, fd, ev);
	if (ret == 0)
		ep->event_count++;
	return ret;
}

int epollex_ctl_mod(struct epollex *ep, int fd, struct epoll_event *ev) {
	int ret;
	ret = epoll_ctl(ep->epfd, EPOLL_CTL_MOD, fd, ev);
	if (ret == 0)
		ep->event_count++;
	return ret;
}

int epollex_ctl_del(struct epollex *ep, int fd) {
	int ret;
	ret = epoll_ctl(ep->epfd, EPOLL_CTL_DEL, fd, NULL);
	if (ret == 0)
		ep->event_count++;
	return ret;
}

int epollex_count(struct epollex *ep) {
	return ep->event_count;
}


