/*
 * list_queue.h
 *
 *  Created on: 2017年3月29日
 *      Author: cj
 * 
 * struct test {
 *    struct list_head list;
 *    int v;
 * };
 *
 * struct test v;
 *
 * struct list_queue queue;
 * struct list_head *p;
 * list_queue_init(&queue);
 * 
 * v.v=123;
 * list_queue_apppend_tail(&queue, &v.list);
 * 
 * p=list_queue_pop(&queue); //p=(struct list_head *)p;
 * p=list_entry(p,struct test,list);
 * printf("%d\n", p->v);
 *
 * list_queue_release(&queue); //you need clear list
 * 
 * 
 *
 */

#ifndef SRC_LIST_QUEUE_H_
#define SRC_LIST_QUEUE_H_

#include <pthread.h>
#include <semaphore.h>
#include "list.h"

#define CONFIG_LIST_USE_CONDLOCK

struct list_queue
{
	pthread_mutex_t lock;

#ifdef CONFIG_LIST_USE_CONDLOCK //条件变量
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_condattr_t attr;
#if 0
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	pthread_cond_init(&cond, &attr);

	pthread_mutex_lock(&mutex);/*锁住互斥量*/
	//pthread_cond_broadcast(&cond);
	pthread_cond_signal(&cond);/*条件改变，发送信号，通知t_b进程*/ 
	pthread_mutex_unlock(&mutex);/*解锁互斥量*/

	pthread_mutex_lock(&mutex); 
	pthread_cond_wait(&cond,&mutex);/*解锁mutex，并等待cond改变*/ 
	pthread_mutex_unlock(&mutex); 

	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec += sec;
	pthread_mutex_lock(&mutex);/*锁住互斥量*/
	ret = pthread_cond_timedwait(&cond, &mutex, &tv);
	pthread_mutex_unlock(&mutex);/*解锁互斥量*/
#endif
	
#else
	//有名信号量sem_open/sem_close
	//内存信号量sem_init/sem_destroy
	sem_t sem;
	// sem_getvalue(&list.sem, &value);
#endif

	struct list_head list;
	int length;
};

/* init */
void list_queue_init(struct list_queue *queue);

void list_queue_append_head(struct list_queue *queue, struct list_head *newlist);

void list_queue_append_tail(struct list_queue *queue, struct list_head *newlist);

//postflag: 1 send sem; 0 not send signed
void list_queue_append_tail2(struct list_queue *queue,
							 struct list_head *newlist, int postflag);

void list_queue_release(struct list_queue *queue);

int list_queue_length(struct list_queue *queue);

int list_queue_wait(struct list_queue *queue, int ms);

//if you need set os time, not use the fun
//如果需要修改系统时间，就不能使用这个函数,请使用 list_queue_wait
int list_queue_timedwait(struct list_queue *queue, int seconds);

struct list_head *list_queue_pop(struct list_queue *queue);

struct list_head *list_queue_first(struct list_queue *queue);

struct list_head *list_queue_next(struct list_queue *queue,
								  struct list_head *mylist);

void list_queue_list_delete(struct list_queue *queue, struct list_head *mylist);

int list_queue_lock(struct list_queue *queue);
int list_queue_unlock(struct list_queue *queue);

/**
 * sem wait expand
 *   sem_trywait  
 *   usleep(1000)  and errno ==EAGAIN
 * 1S=1000ms  
 * 1ms=1000us
 * 1us=1000ns
 * @ms >10
 */
int sem_wait_ms(sem_t *sem, int ms);

#endif /* SRC_LIST_QUEUE_H_ */
