/*
 * WebSocketClient.c
 *
 *  Created on: 2019
 *      Author: cc
 */
#include <time.h>
#include "WebSocketClient.h"

struct WebSocketMsg *WebSocketMsgCalloc()
{
	struct WebSocketMsg *item = (struct WebSocketMsg*) malloc(sizeof(struct WebSocketMsg));
	if(item)
		memset(item, 0, sizeof(struct WebSocketMsg));
	return item;
}

void WebSocketMsgFree(struct WebSocketMsg *item)
{
	if (!item)
		return;
	if (item->data)
		free(item->data);
	free(item);
}

struct WebSocketMsg *WebSocketMsgNewCopy(const void *data, int dlen)
{
	struct WebSocketMsg *item = WebSocketMsgCalloc();
	if (!item)
		return item;
	item->data = malloc(dlen);
	if (!item->data)
	{
		WebSocketMsgFree(item);
		return NULL;
	}
	item->dlen = dlen;
	memcpy(item->data, data, dlen);
	return item;
}

struct WebSocketMsg *WebSocketMsgNewSet(void *data, int dlen)
{
	struct WebSocketMsg *item = WebSocketMsgCalloc();
	if (!item)
		return item;
	item->data = data;
	item->dlen = dlen;
	return item;
}

struct WebSocketMsg *WebSocketMsgNewLen(int dlen)
{
	struct WebSocketMsg *item = WebSocketMsgCalloc();
	if (!item)
		return item;
	item->data = malloc(dlen);
	if (!item->data)
	{
		WebSocketMsgFree(item);
		return NULL;
	}
	item->dlen = dlen;
	return item;
}

void WebSocketMsgQueueInit(struct WebSocketMsgQueue *queue)
{
	pthread_mutex_init(&queue->msglistLock, NULL);
	INIT_LIST_HEAD(&queue->msglist);
	queue->msglistLength = 0;
}

void WebSocketMsgQueueDestroy(struct WebSocketMsgQueue *queue)
{
	struct list_head *p = NULL, *n = NULL;
	struct WebSocketMsg *item;
	pthread_mutex_lock(&queue->msglistLock);
	list_for_each_safe(p,n,&queue->msglist)
	{
		item = list_entry(p, struct WebSocketMsg, list);
		list_del(p);
		if (item->data)
			free(item->data);
		free(item);
	}
	pthread_mutex_unlock(&queue->msglistLock);
	pthread_mutex_destroy(&queue->msglistLock);
}

struct WebSocketMsg * WebSocketMsgQueuePop(struct WebSocketMsgQueue *queue)
{
	struct list_head *p = NULL, *n = NULL;
	struct WebSocketMsg *item = NULL;
	pthread_mutex_lock(&queue->msglistLock);
	list_for_each_safe(p,n,&queue->msglist)
	{
		item = list_entry(p, struct WebSocketMsg, list);
		list_del(p);
		queue->msglistLength--;
		break;
	}
	pthread_mutex_unlock(&queue->msglistLock);
	return item;
}

int WebSocketMsgQueueAppend(struct WebSocketMsgQueue *queue,
		struct WebSocketMsg *item)
{
	pthread_mutex_lock(&queue->msglistLock);
	list_add_tail(&item->list, &queue->msglist);
	queue->msglistLength++;
	pthread_mutex_unlock(&queue->msglistLock);
	return 0;
}

int WebSocketMsgQueueLength(struct WebSocketMsgQueue *queue)
{
	int length = 0;
	pthread_mutex_lock(&queue->msglistLock);
	length = queue->msglistLength;
	pthread_mutex_unlock(&queue->msglistLock);
	return length;
}

void WebSocketClientInit(WebSocketClient *client)
{
	memset(client, 0, sizeof(WebSocketClient));
	pthread_mutex_init(&client->mutex, NULL);
	WebSocketMsgQueueInit(&client->queueWrite);
	client->lastRxTime=time(NULL);
}

void WebSocketClientDestroy(WebSocketClient *client)
{
	WebSocketMsgQueueDestroy(&client->queueWrite);
	pthread_mutex_destroy(&client->mutex);
}

WebSocketClient *WebSocketClientNew()
{
	WebSocketClient *client = (WebSocketClient*) malloc(
			sizeof(WebSocketClient));
	if (!client)
		return client;
	memset(client, 0, sizeof(WebSocketClient));
	WebSocketClientInit(client);
	return client;
}

void WebSocketClientFree(WebSocketClient *client)
{
	if (!client)
		return;
	WebSocketClientDestroy(client);
	free(client);
}

int WebSocketClientWAppend(WebSocketClient *client, const void *data, int dlen)
{
	struct list_head *p, *n;
	struct WebSocketMsg *item = NULL;
	int ret = -1;

	item = WebSocketMsgNewLen(dlen + LWS_PRE);
	if (item)
	{
		memcpy(item->data + LWS_PRE, data, dlen);
		WebSocketMsgQueueAppend(&client->queueWrite, item);
		ret = 0;
	}
	else
	{
		ret = -1;
	}
	return ret;
}
