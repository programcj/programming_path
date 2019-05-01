/*
 * ListQueueCir.c
 *
 *  Created on: 20190428
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ListQueueCir.h"

ListQueueCir *ListQueueCir_New(int maxSize) {
	ListQueueCir *v = (ListQueueCir*) calloc(1, sizeof(ListQueueCir));
	if (!v)
		return NULL;
	v->datas = calloc(maxSize, sizeof(ListQueueCirItem));
	if (!v->datas) {
		free(v);
		return NULL;
	}
	pthread_mutex_init(&v->dataLock, NULL);
	v->maxSize = maxSize;
	return v;
}

void ListQueueCir_Delete(ListQueueCir *queue) {
	int i = 0;
	if (!queue)
		return;
	pthread_mutex_lock(&queue->dataLock);
	for (i = 0; i < queue->maxSize; i++) {
		if (queue->datas[i].data.vptr)
			free(queue->datas[i].data.vptr);
	}
	free(queue->datas);
	pthread_mutex_unlock(&queue->dataLock);
	pthread_mutex_destroy(&queue->dataLock);
	free(queue);
}

int ListQueueCir_Add(ListQueueCir *q, void *data, int dlen) {
	return ListQueueCir_Add2(q, data, dlen, 0);
}

int ListQueueCir_Add2(ListQueueCir *q, void *data, int dlen, int sigflg) {
	int ret = 0;
	ListQueueCirItem *item = NULL;

	//一是添加一个參数，用来记录数组中当前元素的个数；
	//第二个办法是，少用一个存储空间，也就是数组的最后一个存数空间不用，当（rear+1）%maxsiz=front时，队列满；
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
		printf("set data[%d] \n", q->rear);
		memcpy(item->data.vptr, data, dlen);
		item->dlen = dlen;
		q->rear = (q->rear + 1) % q->maxSize;
		q->length++;
	} else {
		ret = -1;
	}
	pthread_mutex_unlock(&q->dataLock);
	return ret;
}

ListQueueCirDataPtr *ListQueueCir_Pop(ListQueueCir *q) {
	ListQueueCirItem *item = NULL;
	pthread_mutex_lock(&q->dataLock);
	if (q->length > 0) {
		item = &q->datas[q->front];
		q->front = (q->front + 1) % q->maxSize;
		q->length--;
	}
	pthread_mutex_unlock(&q->dataLock);
	return item ? &item->data: 0;
}

ListQueueCirItem *ListQueueCir_PopItem(ListQueueCir *q) {
	ListQueueCirItem *item = NULL;
	pthread_mutex_lock(&q->dataLock);
	if (q->length > 0) {
		item = &q->datas[q->front];
		q->front = (q->front + 1) % q->maxSize;
		q->length--;
	}
	pthread_mutex_unlock(&q->dataLock);
	return item;
}

int ListQueueCir_length(ListQueueCir *q) {
	int len = 0;
	pthread_mutex_lock(&q->dataLock);
	len = q->length;
	pthread_mutex_unlock(&q->dataLock);
	return len;
}

