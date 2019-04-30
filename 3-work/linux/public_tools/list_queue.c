/*
 * list_queue.c
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
#include "list_queue.h"

void list_queue_init(struct list_queue *queue) {
	INIT_LIST_HEAD(&queue->list);
	pthread_mutex_init(&queue->lock, NULL);
	sem_init(&queue->sem, 0, 0);
	queue->length = 0;
}

void list_queue_append_head(struct list_queue *queue, struct list_head *newlist) {
	pthread_mutex_lock(&queue->lock);
	list_add(newlist, &queue->list);
	queue->length++;
	sem_post(&queue->sem);
	pthread_mutex_unlock(&queue->lock);
}

void list_queue_append_tail(struct list_queue *queue, struct list_head *newlist) {
	int rc = 0;
	pthread_mutex_lock(&queue->lock);
	list_add_tail(newlist, &queue->list);
	queue->length++;
	rc = sem_post(&queue->sem);
	if (rc != 0) {
		printf("[list_queue_append_tail] sem_post err:%s\n", strerror(errno));
	}
	pthread_mutex_unlock(&queue->lock);
}

void list_queue_append_tail2(struct list_queue *queue, struct list_head *newlist, int postflag) {
	int rc = 0;
	pthread_mutex_lock(&queue->lock);
	list_add_tail(newlist, &queue->list);
	queue->length++;
	if (postflag)
		rc = sem_post(&queue->sem);
	if (rc != 0) {
		printf("[list_queue_append_tail2] sem_post err:%s\n", strerror(errno));
	}
	pthread_mutex_unlock(&queue->lock);
}

void list_queue_release(struct list_queue *queue) {
	pthread_mutex_lock(&queue->lock);
	pthread_mutex_unlock(&queue->lock);

	pthread_mutex_destroy(&queue->lock);
	sem_destroy(&queue->sem);
}

int list_queue_length(struct list_queue *queue) {
	int length = 0;
	pthread_mutex_lock(&queue->lock);
	length = queue->length;
	pthread_mutex_unlock(&queue->lock);
	return length;
}

int sem_wait_ms(sem_t *sem, int ms) {
	int i = 0;
	int interval = 10000; //10ms   1s=1000ms   sizeof(int)=4   0-4294967295    2147483647  (1000 0000)
	int count = (1000.0 * ms) / interval;
	int rc = -1;
	
	////sem_trywait是一个立即返回函数，不会因为任何事情阻塞。
	//如果返回值为0，说明信号量在该函数调用之前大于0，但是调用之后会被该函数自动减1.至于调用之后是否为零则不得而知了。
	//如果返回值为EAGAIN说明信号量计数为0。
	//只是如果递减不能立即执行，调用将返回错误(errno 设置为EAGAIN)而不是阻塞
	while (++i < count && (rc = sem_trywait(sem)) != 0) { 
		//printf("rc=%d\n", rc);
		if (rc == -1 && ((rc = errno) != EAGAIN)) {
			rc = -1;
			break;
		}
		usleep(interval);
	}
	
	if(rc==EAGAIN) {
		rc=-1;
		errno=ETIMEDOUT;
	}
	return rc;
}

int list_queue_wait(struct list_queue *queue, int ms) {
	//return Thread_SemWait(&queue->sem, seconds * 100);
//	int i = 0;
//	int interval = 10000; //10ms   1s=1000ms   sizeof(int)=4   0-4294967295    2147483647  (1000 0000)
//	int count = (1000.0 * ms) / interval;
//	int rc = -1;
//	//if not close
//
//	while (++i < count && (rc = sem_trywait(&queue->sem)) != 0) {
//		if (rc == -1 && ((rc = errno) != EAGAIN)) {
//			rc = 0;
//			break;
//		}
//		usleep(interval);
//	}
//	return rc;
	return sem_wait_ms(&queue->sem, ms);
}

int list_queue_timedwait(struct list_queue *queue, int seconds) {
	struct timespec ts;
	struct timeval tt;
	gettimeofday(&tt, NULL);
	ts.tv_sec = tt.tv_sec;
	ts.tv_nsec = tt.tv_usec * 1000;
	//clock_gettime(CLOCK_REALTIME, &ts);

	ts.tv_sec += seconds;
	ts.tv_nsec = 0;

	return sem_timedwait(&queue->sem, &ts); //会被信号中断
}

struct list_head *list_queue_pop(struct list_queue *queue) {
	struct list_head *p = NULL, *n = NULL;
	struct list_head *v = NULL;

	pthread_mutex_lock(&queue->lock);
	list_for_each_safe(p,n,&queue->list)
	{
		v = p;
		list_del(p);
		queue->length--;
		break;
	}
	pthread_mutex_unlock(&queue->lock);
	return v;
}

struct list_head *list_queue_first(struct list_queue *queue) {
	struct list_head *p = NULL, *n = NULL;
	struct list_head *v = NULL;

	pthread_mutex_lock(&queue->lock);
	list_for_each_safe(p,n,&queue->list)
	{
		v = p;
		break;
	}
	pthread_mutex_unlock(&queue->lock);
	return v;
}

struct list_head *list_queue_next(struct list_queue *queue, struct list_head *mylist) {
	struct list_head *p = NULL;
	struct list_head *v = NULL;
	pthread_mutex_lock(&queue->lock);

	if (mylist) {
		p = mylist->next;
	} else {
		p = (&queue->list)->next;
	}
	if (p != (&queue->list)) {
		v = p;
	}
	pthread_mutex_unlock(&queue->lock);
	return v;
}

void list_queue_list_delete(struct list_queue *queue, struct list_head *mylist) {
	pthread_mutex_lock(&queue->lock);
	list_del(mylist);
	queue->length--;
	pthread_mutex_unlock(&queue->lock);
}

int list_queue_lock(struct list_queue *queue){
	return pthread_mutex_lock(&queue->lock);
}

int list_queue_unlock(struct list_queue *queue){
	return pthread_mutex_unlock(&queue->lock);
}
