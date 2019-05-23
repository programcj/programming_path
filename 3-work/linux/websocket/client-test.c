/*
 * WebSocketClient.c
 *
 *  Created on: 2019年5月20日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "WebSocketClient.h"
#include "LxWebSocketClient.h"
#include "libwebsockets.h"

#define RX_BUFFER_SIZE (1024*200)

static struct lws *_wsiclient = NULL;
static WebSocketClient *_webSocketClient = NULL;

static int _webSockServiceCallback(struct lws *wsi,
		enum lws_callback_reasons reason, void *user, void *in, size_t len) {
	WebSocketClient *webSocketClient = (WebSocketClient*) user;
	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		printf("INIT %p\n", wsi);

		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		printf("LWS_CALLBACK_CLIENT_CONNECTION_ERROR %p\n", wsi);
		break;
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		printf("LWS_CALLBACK_CLIENT_ESTABLISHED %p\n", wsi);
		_webSocketClient = webSocketClient;
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
		printf("LWS_CALLBACK_CLIENT_RECEIVE\n");
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE:
		printf("LWS_CALLBACK_CLIENT_WRITEABLE\n");
		break;
	case LWS_CALLBACK_CLOSED:
		printf("LWS_CALLBACK_CLOSED %p \n", wsi);
		break;
	case LWS_CALLBACK_WSI_DESTROY:
		printf("LWS_CALLBACK_WSI_DESTROY %p \n", wsi);
		{
			if (_webSocketClient == webSocketClient) {
				_webSocketClient = NULL;
				_wsiclient = NULL;
			}
			if (!webSocketClient) {

			}
		}
		break;
	case LWS_CALLBACK_LOCK_POLL:
		break;
	case LWS_CALLBACK_UNLOCK_POLL:
		if (webSocketClient->lastWrTime > 0) {
			if (abs(time(NULL) - webSocketClient->lastWrTime) > 10) { //如果10秒还没发送数据,数据发送不出去
				logd("close......\n");
				shutdown(lws_get_socket_fd(wsi), SHUT_WR); //只能这样关闭
				//lws_cancel_service()
				return -1;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

static struct lws_protocols protocols[] = { { "myproto",
		_webSockServiceCallback, sizeof(WebSocketClient), 1024 * 100 }, { NULL,
NULL, 0, 0 } /* end */
};

void LxWebSocketClientLoop() {
	struct lws_context_creation_info info;
	struct lws_context *context = NULL;
	struct lws_client_connect_info i;

	memset(&info, 0, sizeof info);

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.iface = NULL;
	info.protocols = protocols;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = NULL; 	//lws_get_internal_extensions();
	info.gid = -1;
	info.uid = -1;
	info.options = 0;

	context = lws_create_context(&info);

	memset(&i, 0, sizeof(i));

	i.path = "/";
	i.address = "192.168.0.88"; //192.168.0.81
	i.port = 10086; //
	i.context = context;
	i.ssl_connection = 0;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = -1;
	i.client_exts = NULL;
	i.protocol = "myproto";

	time_t tmstart = time(NULL);
	_wsiclient = lws_client_connect_via_info(&i);

	printf("_wsiclient=%p\n", _wsiclient);

	while (1) {
		if (_wsiclient == NULL && abs(time(NULL) - tmstart) > 5) {
			printf("need restart connect ....\n");
			tmstart = time(NULL);
			_wsiclient = lws_client_connect_via_info(&i);
		}
		lws_service(context, 200);
	}

	lws_context_destroy(context);

}

