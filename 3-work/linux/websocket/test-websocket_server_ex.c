/*
 ============================================================================
 Name        : test_websocket.c
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "libwebsockets.h"
#include <pthread.h>
#include "clog.h"
#include "list.h"  //kernel list.h
#include "WebSocketClient.h"

#define PROTOCOL_NAME "hello"
#define PROTOCOL_PORT 800
#define RX_BUFFER_SIZE 48 * 1024

LIST_HEAD(websocketClients);

void WebSocketClientAdd(WebSocketClient *client) {
	list_add(&client->list, &websocketClients);
}

void WebSocketClientDelete(WebSocketClient *client) {
	list_del(&client->list);
}

static int _websocket_callback(struct lws *wsi,
		enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	WebSocketClient *websockClient = (WebSocketClient*) user;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT: {

	}
		break;
	case LWS_CALLBACK_PROTOCOL_DESTROY: {

	}
		breka;
	case LWS_CALLBACK_ESTABLISHED:
		log_d("LWS_CALLBACK_ESTABLISHED=%d", reason);
		{
			memset(websockClient, 0, sizeof(WebSocketClient));
			websockClient->wsi = wsi;
			pthread_mutex_init(&websockClient->mutex, NULL);
			WebSocketMsgQueueInit(&websockClient->queueWrite);

			WebSocketClientAdd(websockClient);

			const char *data =
					">>3333333333333333333333333333333333333333333333333333333333333AA<<";
			struct WebSocketMsg *item = WebSocketMsgNewCopy(data + LWS_PRE,
					strlen(data) + LWS_PRE);
			WebSocketMsgQueueAppend(&websockClient->queueWrite, item);
		}
		break;
	case LWS_CALLBACK_CLOSED:
		log_d("LWS_CALLBACK_CLOSED=%d", reason);
		break;
	case LWS_CALLBACK_WSI_DESTROY:
		log_d("LWS_CALLBACK_WSI_DESTROY=%d", reason);
		{
			if (websockClient) {
				WebSocketMsgQueueDestroy(&websockClient->queueWrite);
				pthread_mutex_destroy(&websockClient->mutex);
				WebSocketClientDelete(websockClient);
			}
		}
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE: {
		struct WebSocketMsg *item = NULL;
		int wlen = 0;
		item = WebSocketMsgQueuePop(&websockClient->queueWrite); //

		log_d("write......");
		if (item) {
			/**
			 * lws_write 会处理调用send导致的errno为 
			 *    EAGAIN  EWOULDBLOCK  EINTR 可以参考
			 *  https://github.com/warmcat/libwebsockets/blob/master/lib/core-net/output.c
			 *  貌似发送此事件时会返回为0, 错误会返回-1
			 *  然后把待发送的数据放入lws的buflist_out中,自动添加lws_callback_on_writable,下次先判断lws的buflist_out
			 *     buflist_out见 https://github.com/warmcat/libwebsockets/blob/master/lib/core-net/private.h
			 *     老版本中不是buflist_out,没有 lws_buflist 
			 * 
			 * https://github.com/warmcat/libwebsockets/blob/master/minimal-examples/ws-server/minimal-ws-server-threads/protocol_lws_minimal.c
			 * LWS_CALLBACK_EVENT_WAIT_CANCELLED
			 * lws_cancel_service(vhd->context); 退出阻塞
			 * 
			 * https://blog.csdn.net/sonysuqin/article/details/82050970
			 * 在调用线程中调用lws_cancel_service方法强制lws_service退出阻塞
			 * 
			 */
			wlen = lws_write(wsi, (unsigned char *) (item->data + LWS_PRE),
					item->dlen - LWS_PRE, LWS_WRITE_BINARY);

			if (wlen < 0)
				lws_close_reason(wsi, LWS_CLOSE_STATUS_NOSTATUS,
						(unsigned char *) "done", 4);
		}

//		if (lws_send_pipe_choked(wsi)) { //如果ws连接阻塞，则返回1，否则返回0
//			lws_callback_on_writable(wsi);
//			break;
//		}
		WebSocketMsgFree(item);

		//创建消息添加到发送消息队列
		item = WebSocketMsgNewLen(100 + LWS_PRE);
		if (item) {
			memset(item->data + LWS_PRE, 'b', item->dlen - LWS_PRE);
			WebSocketMsgQueueAppend(&websockClient->queueWrite, item);
		}
	}
		break;
	case LWS_CALLBACK_RECEIVE:
		log_d("LWS_CALLBACK_RECEIVE %s", (char* )in);
		const char *data = ">>bdsA<<";
		struct WebSocketMsg *item = WebSocketMsgNewCopy(data + LWS_PRE,
				strlen(data) + LWS_PRE);
		WebSocketMsgQueueAppend(&websockClient->queueWrite, item);
		break;
	}

	return 0;
}

int main(void) {
	struct lws_context_creation_info info;
	struct lws_protocols protocol;
	struct lws_context *context = NULL;
	int ret = 0;
	info.port = PROTOCOL_PORT; //CONTEXT_PORT_NO_LISTEN;//PROTOCOL_PORT;
	info.iface = NULL;
	info.protocols = &protocol;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = NULL; //lws_get_internal_extensions();
	info.gid = -1;
	info.uid = -1;
	info.options = 0;

	protocol.name = PROTOCOL_NAME;
	protocol.callback = &_websocket_callback;
	protocol.per_session_data_size = sizeof(WebSocketClient);
	protocol.rx_buffer_size = RX_BUFFER_SIZE;
	protocol.id = 0;
	protocol.user = NULL;

	context = lws_create_context(&info);

	struct list_head *p = NULL, *n = NULL;
	WebSocketClient *client;

	while (ret >= 0) {
		list_for_each_safe(p,n,&websocketClients)
		{
			client = list_entry(p, WebSocketClient, list);
			if (WebSocketMsgQueueLength(&client->queueWrite) > 0) {
				log_d("need write...");
				lws_callback_on_writable(client->wsi);
			}
		}
		ret = lws_service(context, 100);
	}

	lws_context_destroy(context);

	return EXIT_SUCCESS;
}
