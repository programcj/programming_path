/*
 * WebSocketClient.h
 *
 *  Created on: 2019��3��30��
 *      Author: cc
 */

#ifndef SRC_LIBSRC_LIBCOMMUNICATE_WEBSOCKETCLIENT_H_
#define SRC_LIBSRC_LIBCOMMUNICATE_WEBSOCKETCLIENT_H_

#include <pthread.h>
#include "list.h"
#include "libwebsockets.h"

struct WebSocketMsg
{
	struct list_head list;
	void *data;
	int dlen;
};

struct WebSocketMsgQueue
{
	pthread_mutex_t msglistLock;
	struct list_head msglist;
	int msglistLength;
};

typedef struct _WebSocketClient
{
	struct list_head list;  /*need linux kernel list.h*/
	struct lws *wsi;  /* libwebsockets wsi*/
	int uuid;   /*client id*/
	int lastRxTime; /* last read data time*/
	int lastWrTime; /* last write data time*/
	struct WebSocketMsgQueue queueWrite; /* need write data list*/
	pthread_mutex_t mutex;   /*mutex lock*/
	const void *bindData; /*Bind data*/
} WebSocketClient;

#define WebSocketClientGetBindData(client, type) ((type *)(client->bindData))

struct WebSocketMsg *WebSocketMsgCalloc();

void WebSocketMsgFree(struct WebSocketMsg *item);

struct WebSocketMsg *WebSocketMsgNewCopy(const void *data, int dlen);

struct WebSocketMsg *WebSocketMsgNewSet(void *data, int dlen);

struct WebSocketMsg *WebSocketMsgNewLen(int dlen);

void WebSocketMsgQueueInit(struct WebSocketMsgQueue *queue);

void WebSocketMsgQueueDestroy(struct WebSocketMsgQueue *queue);

struct WebSocketMsg * WebSocketMsgQueuePop(struct WebSocketMsgQueue *queue);

int WebSocketMsgQueueAppend(struct WebSocketMsgQueue *queue,
		struct WebSocketMsg *item);

int WebSocketMsgQueueLength(struct WebSocketMsgQueue *queue);

void WebSocketClientInit(WebSocketClient *client);
void WebSocketClientDestroy(WebSocketClient *client);

WebSocketClient *WebSocketClientNew();
void WebSocketClientFree(WebSocketClient *client);
int WebSocketClientWAppend(WebSocketClient *client, const void *data, int dlen);

#endif /* SRC_LIBSRC_LIBCOMMUNICATE_WEBSOCKETCLIENT_H_ */