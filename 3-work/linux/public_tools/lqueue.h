/*
 * lqueue.h
 *
 *  Created on: 2017年3月29日
 *      Author: cj
 * 
 * struct lqueue queue;
 * 
 * lqueue_init(&queue);
 * 
 * lqueue_release(&queue); //you need clear list
 * 
 * 
 */

#ifndef _LQUEUE_H_
#define _LQUEUE_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

//Pthreads
//spinlock  自旋锁
//rwlock 	读写锁
//Mutex 	互斥量
//Cond 		条件变量
//Semaphore 信号变量

//>kernel
//spin_lock 自旋锁
//原子变量
//dy_lock 	临界区
//read_lock 读写自旋锁
//DECLARE_MUTEX sema_init 信号量
//DECLARE_COMPLETION 	完成变量
//wakelock机制
//Per-CPU变量
struct os_event_cond {
    int initflag;
    pthread_mutex_t smutex;
    pthread_cond_t scond;
    pthread_condattr_t sattr;
};

struct os_event_cond* os_event_cond_new();
void os_event_cond_delete(struct os_event_cond* p);

int os_event_cond_init(struct os_event_cond* p);

void os_event_cond_destory(struct os_event_cond* p);

int os_event_cond_notify(struct os_event_cond* p);

int os_event_cond_notify_all(struct os_event_cond* p);

int os_event_cond_wait_sec(struct os_event_cond* p, int sec);

struct lqueue_node {
    struct lqueue_node* next;
    struct lqueue_node* priv;
    struct lqueue* queue;
};

#define lqueue_node_entry(ptr, type, node) \
    (type*)((char*)ptr - (size_t) & ((type*)0)->node)

struct lqueue {
    //head
    struct lqueue_node* next;
    struct lqueue_node* priv;
    long lqlen;
    pthread_mutex_t lock;
    void* user; //绑定数据

    //#ifdef CONFIG_LIST_USE_CONDLOCK //条件变量
    //	pthread_mutex_t smutex;
    //	pthread_cond_t scond;
    //	pthread_condattr_t sattr;
    //	pthread_condattr_init(&attr);
    //	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    //	pthread_cond_init(&cond, &attr);
    //
    //	pthread_mutex_lock(&mutex);/*锁住互斥量*/
    ////pthread_cond_broadcast(&cond);
    //	pthread_cond_signal(&cond);/*条件改变，发送信号，通知t_b进程*/
    //	pthread_mutex_unlock(&mutex);/*解锁互斥量*/
    //
    //	pthread_mutex_lock(&mutex);
    //	pthread_cond_wait(&cond,&mutex);/*解锁mutex，并等待cond改变*/
    //	pthread_mutex_unlock(&mutex);
    //
    //	struct timespec tv;
    //	clock_gettime(CLOCK_MONOTONIC, &tv);
    //	tv.tv_sec += sec;
    //	pthread_mutex_lock(&mutex);/*锁住互斥量*/
    //	ret = pthread_cond_timedwait(&cond, &mutex, &tv);
    //	pthread_mutex_unlock(&mutex);/*解锁互斥量*/
    //	pthread_cond_destroy(&queue->scond);
    //	pthread_condattr_destroy(&queue->sattr);
    //	pthread_mutex_destroy(&queue->smutex);
    //有名信号量sem_open/sem_close
    //内存信号量sem_init/sem_destroy
    //	sem_t sem;
    // sem_getvalue(&list.sem, &value);
};

static inline void lqueue_init(struct lqueue* queue)
{
    pthread_mutex_init(&queue->lock, NULL);
    queue->next = queue->priv = (struct lqueue_node*)queue;
    queue->lqlen = 0;
}

static inline void lqueue_destory(struct lqueue* queue)
{
    pthread_mutex_destroy(&queue->lock);
    queue->lqlen = 0;
}

static inline long lqueue_len(struct lqueue* queue)
{
    return queue->lqlen;
}

static inline void lqueue_insert(struct lqueue* queue,
    struct lqueue_node* newnode, struct lqueue_node* prev,
    struct lqueue_node* next)
{
    newnode->next = next;
    newnode->priv = prev;
    next->priv = prev->next = newnode;
    newnode->queue = queue;
    queue->lqlen++;
}

static inline void lqueue_node_del(struct lqueue_node* node)
{
    node->priv->next = node->next;
    node->next->priv = node->priv;
    node->priv = node->next = NULL;
    node->queue->lqlen--;
}

static inline void lqueue_append_tail(struct lqueue* queue,
    struct lqueue_node* newnode)
{
    pthread_mutex_lock(&queue->lock);
    lqueue_insert(queue, newnode, queue->priv, (struct lqueue_node*)queue);
    pthread_mutex_unlock(&queue->lock);
}

static inline void lqueue_append_head(struct lqueue* queue,
    struct lqueue_node* newnode)
{
    pthread_mutex_lock(&queue->lock);
    lqueue_insert(queue, newnode, (struct lqueue_node*)queue, queue->next);
    pthread_mutex_unlock(&queue->lock);
}

static inline struct lqueue_node* lqueue_pop(struct lqueue* queue)
{
    struct lqueue_node* node = NULL;
    pthread_mutex_lock(&queue->lock);
    node = queue->priv;

    if (node == (struct lqueue_node*)queue)
        node = NULL;
    else {
        node->priv->next = node->next;
        node->next->priv = node->priv;

        node->priv = node->next = NULL;
        node->queue = NULL;
        queue->lqlen--;
    }
    pthread_mutex_unlock(&queue->lock);
    return node;
}

static inline struct lqueue_node* lqueue_dequeue(struct lqueue* queue)
{
    struct lqueue_node* node = NULL;
    pthread_mutex_lock(&queue->lock);
    node = queue->next;
    if (node == (struct lqueue_node*)queue)
        node = NULL;
    else {
        lqueue_node_del(node);
    }
    pthread_mutex_unlock(&queue->lock);
    return node;
}

//非加锁方式出队
static inline struct lqueue_node* lqueue_nolock_dequeue(struct lqueue* queue)
{
	struct lqueue_node* node = NULL;
	node = queue->next;
    if (node == (struct lqueue_node*)queue)
        node = NULL;
    else 
        lqueue_node_del(node);
    return node;
}

#endif /* SRC_lqueue_H_ */
