/*
 ============================================================================
 Name        : test-websocket.c
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include "clog.h"
#include <pthread.h>

struct session_data {
	char name[100];
};

static int ws_service_callback(struct lws *wsi,
		enum lws_callback_reasons reason, void *user, void *in, size_t size) {
	int n;
	struct session_data *session = (struct session_data*) user;

	switch (reason) {
	case LWS_CALLBACK_GET_THREAD_ID:
		break;
	case LWS_CALLBACK_CLIENT_ESTABLISHED: {
		log_d("CLIENT_ESTABLISHED");
		char *str = "hello start..";
		int len = strlen(str);
		unsigned char *out = (unsigned char *) malloc(
				sizeof(unsigned char)
						* (LWS_SEND_BUFFER_PRE_PADDING + len
								+ LWS_SEND_BUFFER_POST_PADDING));
		memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);
		n = lws_write(wsi, out + LWS_SEND_BUFFER_PRE_PADDING, len,
				LWS_WRITE_TEXT);

		//sprintf(session->name, "start....");
		//log_d("n=%d", n);
	}
		break;
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		log_d("CONNECTION_ERROR");
		return -1;
		break;
	case LWS_CALLBACK_CLOSED:
		log_d("LWS_CALLBACK_CLOSED");
		break;
	case LWS_CALLBACK_WSI_DESTROY:
		log_d("LWS_CALLBACK_WSI_DESTROY");
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		log_d("session:%s, recv(len=%d)>%s", session->name, size, (char * ) in);
		{
			char *str = "hello world. recv";
			int len = strlen(str);
			unsigned char *out = (unsigned char *) malloc(
					sizeof(unsigned char) * (LWS_PRE + len));
			memcpy(out + LWS_PRE, str, len);

			n = lws_write(wsi, out + LWS_PRE, len, LWS_WRITE_BINARY);
			//log_d("n=%d", n);
			n = lws_callback_on_writable(wsi);
			//log_d("n=%d", n);
		}
		break;
	case LWS_CALLBACK_CLIENT_WRITEABLE: {
		//log_d("LWS_CALLBACK_CLIENT_WRITEABLE");
		char *str = "write.....";
		int len = strlen(str);
		unsigned char *out = (unsigned char *) malloc(
				sizeof(unsigned char) * (LWS_PRE + len));
		memcpy(out + LWS_PRE, str, len);

		n = lws_write(wsi, out + LWS_PRE, len, LWS_WRITE_BINARY);
		n = lws_callback_on_writable(wsi);
	}
		break;
	default:
		break;
	}

	return 0;
}
#ifdef WCLIENT
int main(int argc,char **argv) {
#else
int main_client() {
#endif
	const char *address = "127.0.0.1";
	struct lws_context *context = NULL;
	struct lws_context_creation_info info;
	struct lws *wsi = NULL;
	struct lws_protocols protocol;
//	struct sigaction act;
//	act.sa_handler = INT_HANDLER;
//	act.sa_flags = 0;
//	sigemptyset(&act.sa_mask);
//	sigaction( SIGINT, &act, 0);

	memset(&info, 0, sizeof info);
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.iface = NULL;
	info.protocols = &protocol;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.extensions = NULL;
	info.gid = -1;
	info.uid = -1;
	info.options = 0;

	protocol.name = "abc";
	protocol.callback = &ws_service_callback;
	protocol.per_session_data_size = sizeof(struct session_data);
	protocol.rx_buffer_size = 65535;
	protocol.id = 0;
	protocol.user = NULL;

	context = lws_create_context(&info);
	log_d("[Main] context created.");

	if (context == NULL) {
		log_d("[Main] context is NULL.");
		return -1;
	}

#if 0
	wsi = lws_client_connect(context, address, 1883, 0, "/123",
			"111:5000",
			NULL, protocol.name, -1);
#else
	struct lws_client_connect_info i;
	memset(&i, 0, sizeof(i));
	i.port = 8080;
	i.address = address;
	i.path = "abc";
	i.context = context;
	i.ssl_connection = 0;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = -1;
	i.client_exts = NULL;

	i.protocol = "abc";
	wsi = lws_client_connect_via_info(&i);
#endif

	if (wsi == NULL) {
		log_d("[Main] wsi create error.");
		return -1;
	}
	log_d("start....");

	int n = 0;
	while (n >= 0) {
		//当连接断开时需要重新连接在这使用标志位
		//if conflag==0  lws_client_connect_via_info
		n = lws_service(context, 50);
	}
	lws_context_destroy(context);
	log_d("close....");
	return EXIT_SUCCESS;
}
