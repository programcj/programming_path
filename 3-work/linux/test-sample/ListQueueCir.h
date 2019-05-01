/*
 * ListCQueue.h
 *
 *  Created on: 20190428
 *      Author: cc
 */

#ifndef SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_
#define SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_

#include "pthread.h"

#define PTRSIZE 8

typedef union {
	void *vptr;
	char *vstr;

#if PTRSIZE==4
	unsigned int valueInt;
#elif PTRSIZE==8
	unsigned long vint;
#endif

} ListQueueCirDataPtr;


typedef struct _ListQueueCirItem {
	ListQueueCirDataPtr data;
	int dlen;
} ListQueueCirItem;

typedef struct _ListQueueCir
{
	ListQueueCirItem *datas;
	pthread_mutex_t dataLock;
	int front;    //start item
	int rear;  //next item
	int maxSize;
	int length;
	//sem
	//pthread_mutex_t mutex;
	//pthread_cond_t cond;
} ListQueueCir;

ListQueueCir *ListQueueCir_New(int maxSize);

void ListQueueCir_Delete(ListQueueCir *queue);

int ListQueueCir_Add(ListQueueCir *queue, void *data, int dlen);

int ListQueueCir_Add2(ListQueueCir *queue, void *data, int dlen, int sigflg);

ListQueueCirItem *ListQueueCir_PopItem(ListQueueCir *queue);

ListQueueCirDataPtr *ListQueueCir_Pop(ListQueueCir *queue);

int ListQueueCir_length(ListQueueCir *queue);

#endif /* SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_ */
