/*
 * ListCQueue.h
 *
 *  Created on: 20190428
 *      Author: cc
 */

#ifndef SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_
#define SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_

#include "pthread.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WORDSIZE

#ifdef __x86_64__
#define __WORDSIZE 64
#else
#define __WORDSIZE 32
#endif

#endif

#define PTRSIZE __WORDSIZE

//其实就是指针
typedef union CLangPointer {
    void* vptr;
    char* vstr;

#if PTRSIZE == 32
    unsigned int vinteger;
#elif PTRSIZE == 64
    unsigned long vinteger;
#endif

} ListQueueCirDataPtr;

#define CPOINT(p,dptr) memcpy(&p,&dptr, 8);
#define CPOINT_FROM(dptr)	(*(long*)&dptr)

//void *ptr="hello";
//ListQueueCirDataPtr v;
//CPointTo(&ptr,&v);
inline static void CPointTo(void* ptr, ListQueueCirDataPtr* v)
{
	//v->vptr=ptr;
	memcpy(v, &ptr, sizeof(ptr));
}

inline static void *CPointFrom(ListQueueCirDataPtr *v){
	return v->vptr;
}

typedef struct _ListQueueCirItem {
    ListQueueCirDataPtr data;
    int dlen;
    void* bindData;
} ListQueueCirItem;

typedef struct _ListQueueCir {
    ListQueueCirItem* datas;
    pthread_mutex_t dataLock;
    int front; //start item
    int rear; //next item
    int maxSize;
    int length;
    //sem

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ListQueueCir;

ListQueueCir* ListQueueCir_New(int maxSize);

void ListQueueCir_Delete(ListQueueCir* queue);

ListQueueCirItem* ListQueueCir_Add(ListQueueCir* queue, void* data, int dlen,
    void* bindData);

ListQueueCirItem* ListQueueCir_Add2(ListQueueCir* queue, void* data, int dlen,
    void* bindData, int sigflg);

//多线程有问题(相同的值)
const ListQueueCirItem* ListQueueCir_PopItem(ListQueueCir* queue);

//多线程有问题(相同的值)
void* ListQueueCir_Pop(ListQueueCir* queue);

//多线程无问题
int ListQueueCir_PopTo(ListQueueCir* queue,
    int (*bkfun)(const ListQueueCirItem* item, void* context),
    void* context);

//多线程有问题(相同的值)
const ListQueueCirItem* ListQueueCir_GetFirst(ListQueueCir* queue);

int ListQueueCir_length(ListQueueCir* queue);

int ListQueueCir_isFull(ListQueueCir* queue);

int ListQueueCir_timeWait(ListQueueCir* queue, int sec);

//锁住queue
int ListQueueCir_lock(ListQueueCir* queue);

//解锁锁住queue
int ListQueueCir_unlock(ListQueueCir* queue);

#ifdef __cplusplus
}
#endif

#endif /* SRC_PLUGINS_LIBSMARTIPC_TVT_LISTQUEUECIR_H_ */
