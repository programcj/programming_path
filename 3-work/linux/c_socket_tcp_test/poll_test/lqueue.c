/*
 * lqueue.c
 *
 *  Created on: 2017年3月29日
 *      Author: cj
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

#include "lqueue.h"

struct os_event_cond *os_event_cond_new() {
	struct os_event_cond *p = (struct os_event_cond*) malloc(
			sizeof(struct os_event_cond));
	if (!p)
		return NULL;
	memset(p, 0, sizeof(struct os_event_cond));
	os_event_cond_init(p);
	return p;
}

void os_event_cond_delete(struct os_event_cond *p) {
	if (!p)
		return;
	os_event_cond_destory(p);
	free(p);
}

int os_event_cond_init(struct os_event_cond *p) {
	int ret = 0;

	ret = pthread_mutex_init(&p->smutex, NULL);
	if (ret != 0)
		return ret;

	ret = pthread_condattr_init(&p->sattr);
	if (ret != 0)
		return ret;

	ret = pthread_condattr_setclock(&p->sattr, CLOCK_MONOTONIC);
	if (ret != 0)
		goto __reterr;

	ret = pthread_cond_init(&p->scond, &p->sattr);
	if (ret != 0)
		goto __reterr;

	p->initflag = 1;
	return ret;

	__reterr: pthread_condattr_destroy(&p->sattr);
	return ret;
}

void os_event_cond_destory(struct os_event_cond *p) {
	if (!p->initflag)
		return;
	pthread_cond_destroy(&p->scond);
	pthread_condattr_destroy(&p->sattr);
	pthread_mutex_destroy(&p->smutex);
	p->initflag = 0;
}

int os_event_cond_signal(struct os_event_cond *p) {
	int ret = 0;
	if (!p->initflag)
		return -1;

	ret = pthread_mutex_lock(&p->smutex);/*锁住互斥量*/
	//pthread_cond_broadcast(&cond);
	ret = pthread_cond_signal(&p->scond);/*条件改变，发送信号，通知t_b进程*/
	ret = pthread_mutex_unlock(&p->smutex);/*解锁互斥量*/
	return ret;
}

int os_event_cond_wait_sec(struct os_event_cond *p, int sec) {
	int ret = 0;
	struct timespec tv;

	if (!p->initflag)
		return -1;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec += sec;
	pthread_mutex_lock(&p->smutex);/*锁住互斥量*/
	if (sec == -1) {
		ret = pthread_cond_wait(&p->scond, &p->smutex);
	} else {
		ret = pthread_cond_timedwait(&p->scond, &p->smutex, &tv);
	}
	pthread_mutex_unlock(&p->smutex);/*解锁互斥量*/

	return ret;
}

void lqueue_test() {
	struct lqueue queue;
	struct abcde {
		struct lqueue_node lqueue_node;
		int v;
	};

	struct abcde nodes[100];

	lqueue_init(&queue);
	for (int i = 0; i < 100; i++) {
		nodes[i].v = i;
		lqueue_append_tail(&queue, &nodes[i].lqueue_node);
	}

	struct abcde *v;
	while (1) {
		v = (struct abcde*) lqueue_pop(&queue);
		if (!v)
			break;
		printf("v=%d ", v->v);
	}

	printf("\n---------------------------\n");

	for (int i = 0; i < 100; i++) {
		nodes[i].v = i;
		lqueue_append_tail(&queue, &nodes[i].lqueue_node);
	}

	while (1) {
		v = (struct abcde*) lqueue_dequeue(&queue);
		if (!v)
			break;
		printf("v=%d ", v->v);
	}
	printf("\n");
	lqueue_destory(&queue);
}

