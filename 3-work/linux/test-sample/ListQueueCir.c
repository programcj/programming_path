/*
 * ListQueueCir.c
 *
 *  Created on: 20190428
 *      Author: cc
 */
#include "ListQueueCir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ListQueueCir* ListQueueCir_New(int maxSize)
{
    ListQueueCir* v = (ListQueueCir*)calloc(1, sizeof(ListQueueCir));
    if (!v)
        return NULL;
    v->datas = (ListQueueCirItem*)calloc(maxSize, sizeof(ListQueueCirItem));
    if (!v->datas) {
        free(v);
        return NULL;
    }
    pthread_mutex_init(&v->dataLock, NULL);
    v->maxSize = maxSize;

    pthread_mutex_init(&v->mutex, NULL);

    pthread_condattr_t attr;

    pthread_condattr_init(&attr);
    //关键注意这里,使用系统运行时间，不使用系统时间
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&v->cond, &attr);
    return v;
}

void ListQueueCir_Delete(ListQueueCir* queue)
{
    int i = 0;
    if (!queue)
        return;
    pthread_mutex_lock(&queue->dataLock);
    for (i = 0; i < queue->maxSize; i++) {
        if (queue->datas[i].data.vptr)
            free(queue->datas[i].data.vptr);
    }
    free(queue->datas);
    queue->maxSize = 0;
    queue->length = 0;
    pthread_mutex_unlock(&queue->dataLock);

    pthread_mutex_destroy(&queue->dataLock);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    free(queue);
}

ListQueueCirItem* ListQueueCir_Add(ListQueueCir* q, void* data, int dlen,
    void* bindData)
{
    return ListQueueCir_Add2(q, data, dlen, bindData, 0);
}

__attribute__((unused)) void* ListQueueCir_Pop(ListQueueCir* q)
{
    ListQueueCirItem* item = NULL;
    pthread_mutex_lock(&q->dataLock);
    if (q->length > 0) {
        item = &q->datas[q->front];
        q->front = (q->front + 1) % q->maxSize;
        q->length--;
    }
    pthread_mutex_unlock(&q->dataLock);

    if (!item)
        return NULL;
    if (sizeof(void*) == sizeof(int))
        return (void*)(*(int*)(&item->data));
    return (void*)*(long*)(&item->data);
}

int ListQueueCir_PopTo(ListQueueCir* q,
    int (*bkfun)(const ListQueueCirItem* item, void* context),
    void* context)
{
    int ret = 0;
    ListQueueCirItem* item = NULL;
    pthread_mutex_lock(&q->dataLock);
    if (q->length > 0) {
        item = &q->datas[q->front];
        ret = bkfun(item, context);
        q->front = (q->front + 1) % q->maxSize;
        q->length--;
    } else {
        ret = -1;
    }
    pthread_mutex_unlock(&q->dataLock);
    return ret;
}

const ListQueueCirItem* ListQueueCir_PopItem(ListQueueCir* q)
{
    ListQueueCirItem* item = NULL;
    pthread_mutex_lock(&q->dataLock);
    if (q->length > 0) {
        item = &q->datas[q->front];
        q->front = (q->front + 1) % q->maxSize;
        q->length--;
    }
    pthread_mutex_unlock(&q->dataLock);
    return item;
}

const ListQueueCirItem* ListQueueCir_GetFirst(ListQueueCir* q)
{
    ListQueueCirItem* item = NULL;
    pthread_mutex_lock(&q->dataLock);
    if (q->length > 0) {
        item = &q->datas[q->front];
    }
    pthread_mutex_unlock(&q->dataLock);
    return item;
}

int ListQueueCir_length(ListQueueCir* q)
{
    int len = 0;
    pthread_mutex_lock(&q->dataLock);
    len = q->length;
    pthread_mutex_unlock(&q->dataLock);
    return len;
}

int ListQueueCir_isFull(ListQueueCir* queue)
{
    return queue->length == queue->maxSize ? 1 : 0;
}

ListQueueCirItem* ListQueueCir_Add2(ListQueueCir* q, void* data, int dlen,
    void* bindData, int sigflg)
{
    ListQueueCirItem* item = NULL;

    pthread_mutex_lock(&q->dataLock);
    if (q->length < q->maxSize) {
        item = &q->datas[q->rear];

        if (dlen > item->dlen) {
            if (item->data.vptr)
                free(q->datas[q->rear].data.vptr);
            item->data.vptr = NULL;
        }

        if (!item->data.vptr) {
            item->data.vptr = malloc(dlen);
        }

        memcpy(item->data.vptr, data, dlen);
        item->dlen = dlen;
        item->bindData = bindData;

        q->rear = (q->rear + 1) % q->maxSize;
        q->length++;

        if (sigflg) {
            pthread_mutex_lock(&q->mutex);
            pthread_cond_signal(&q->cond);
            pthread_mutex_unlock(&q->mutex);
        }
    } else {
        item = NULL;
    }
    pthread_mutex_unlock(&q->dataLock);
    return item;
}

int ListQueueCir_timeWait(ListQueueCir* queue, int sec)
{
    int ret = 0;
    struct timespec tv;

    pthread_mutex_lock(&queue->mutex);
    if (ListQueueCir_length(queue) == 0) {
        //CLOCK_REALTIME ：默认类型，它记录的时间是按照系统的时间计算的,也就是说在计算中有人调整了系统时间,它也会受到影响
        //CLOCK_MONOTONIC ： monotonic time的字面意思是单调时间，实际上它指的是系统启动以后流逝的时间
        clock_gettime(CLOCK_MONOTONIC, &tv);
        tv.tv_sec += sec;
        ret = pthread_cond_timedwait(&queue->cond, &queue->mutex, &tv);
        //ret = pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    pthread_mutex_unlock(&queue->mutex);
    return ret;
}

int ListQueueCir_lock(ListQueueCir* queue)
{
    return pthread_mutex_lock(&queue->dataLock);
}

int ListQueueCir_unlock(ListQueueCir* q)
{
    return pthread_mutex_unlock(&q->dataLock);
}
