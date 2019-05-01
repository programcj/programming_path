/*
 ============================================================================
 Name        : test-websocket.c
 Author      : cj
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 openssl:

#1,CA证书
openssl req -newkey rsa:2048 -nodes -keyout ca_rsa_private.pem -x509 -days 365 -out ca.crt -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=CA/emailAddress=aaaa@qq.com"

#2,服务器私钥与待签名
openssl req -newkey rsa:2048 -nodes -keyout server_rsa_private.pem  -out server.csr -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=SERVER/emailAddress=aaa@qq.com"

#3,使用CA证书及密钥对服务器证书进行签名
openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca_rsa_private.pem -CAcreateserial -out server.crt

#######证书有效检查(SSL_get_verify_result)
openssl verify -CAfile ca.crt server.crt

#4, client
openssl req -newkey rsa:2048 -nodes -keyout client_rsa_private.pem -out client.csr -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=CLIENT/emailAddress=youremail@qq.com"

#5 使用CA证书及密钥对客户端证书进行签名：
openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey ca_rsa_private.pem -CAcreateserial -out client.crt

#######证书有效检查(SSL_get_verify_result)
openssl verify -CAfile ca.crt client.crt

 */

#include <stdio.h>
#include <stdlib.h>
#include "/home/cj/libwebsockets-2.2.0/build/build/install/include/libwebsockets.h"
#include "clog.h"
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

struct session_data {
	char name[100];
};

static int ws_service_callback(struct lws *wsi,
		enum lws_callback_reasons reason, void *user, void *in, size_t size) {
	int n;
	struct session_data *session = (struct session_data*) user;

	switch (reason) {
#ifdef LWS_SSL_CLIENT_CERT_CHECK //双向SSL认证需要
	case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION: {
		X509 *cert;
		if (!len || (SSL_get_verify_result((SSL*) in) != X509_V_OK)) {
			int err = X509_STORE_CTX_get_error((X509_STORE_CTX*) user);
			int depth = X509_STORE_CTX_get_error_depth((X509_STORE_CTX*) user);
			const char* msg = X509_verify_cert_error_string(err);
			log_d(": SSL error: %s (%d), depth: %d\n", msg, err, depth);
			return 1;
		}
	}
	break;
#endif
	case LWS_CALLBACK_GET_THREAD_ID:
		break;
	case LWS_CALLBACK_ESTABLISHED: {
		log_d("ESTABLISHED");
		char *str = "hello my name is server.";
		int len = strlen(str);
		unsigned char *out = (unsigned char *) malloc(
				sizeof(unsigned char)
						* (LWS_SEND_BUFFER_PRE_PADDING + len
								+ LWS_SEND_BUFFER_POST_PADDING));
		memcpy(out + LWS_SEND_BUFFER_PRE_PADDING, str, len);

		n = lws_write(wsi, out + LWS_SEND_BUFFER_PRE_PADDING, len,
				LWS_WRITE_TEXT);

		sprintf(session->name, "start....");
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
	case LWS_CALLBACK_RECEIVE:
		log_d("session:%s, recv(len=%d)>%s", session->name, size, (char * ) in);
		{
			char *str = "hello world. recv";
			int len = strlen(str);
			unsigned char *out = (unsigned char *) malloc(
					sizeof(unsigned char) * (LWS_PRE + len));
			memcpy(out + LWS_PRE, str, len);

			n = lws_write(wsi, out + LWS_PRE, len, LWS_WRITE_TEXT);
			n = lws_callback_on_writable(wsi);
			sleep(1);
		}
		break;
	case LWS_CALLBACK_SERVER_WRITEABLE: {
		//log_d("LWS_CALLBACK_CLIENT_WRITEABLE");
		char *str = "write..server.";
		int len = strlen(str);
		unsigned char *out = (unsigned char *) malloc(
				sizeof(unsigned char) * (LWS_PRE + len));
		memcpy(out + LWS_PRE, str, len);

		n = lws_write(wsi, out + LWS_PRE, len, LWS_WRITE_TEXT);
		n = lws_callback_on_writable(wsi);
		sleep(1);
	}
		break;
	default:
		break;
	}

	return 0;
}

int main(void) {
	struct lws_context *context = NULL;
	struct lws_context_creation_info info;
	struct lws_protocols protocol;

	memset(&info, 0, sizeof info);
	info.port = 8080;
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

#ifdef LWS_SSL_CLIENT_CERT_CHECK  //双向SSL认证需要
	info.ssl_ca_filepath=""; /*CA根证书*/
	info.ssl_cert_filepath=""; /*服务器公钥证书 */
	info.ssl_private_key_filepath=""; /*服务器的私钥*/
	info.options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
#else
	info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.ssl_cert_filepath = "libwebsockets-test-server.pem";
	info.ssl_private_key_filepath = "libwebsockets-test-server.key.pem";
#endif
//	info.options = LWS_SERVER_OPTION_FALLBACK_TO_RAW |
//				LWS_SERVER_OPTION_VALIDATE_UTF8 |
//				LWS_SERVER_OPTION_LIBUV; /* plugins require this */
//	info.options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
//	info.options |= LWS_SERVER_OPTION_REDIRECT_HTTP_TO_HTTPS;

	info.ssl_cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:"
			"ECDHE-RSA-AES256-GCM-SHA384:"
			"DHE-RSA-AES256-GCM-SHA384:"
			"ECDHE-RSA-AES256-SHA384:"
			"HIGH:!aNULL:!eNULL:!EXPORT:"
			"!DES:!MD5:!PSK:!RC4:!HMAC_SHA1:"
			"!SHA1:!DHE-RSA-AES128-GCM-SHA256:"
			"!DHE-RSA-AES128-SHA256:"
			"!AES128-GCM-SHA256:"
			"!AES128-SHA256:"
			"!DHE-RSA-AES256-SHA256:"
			"!AES256-GCM-SHA384:"
			"!AES256-SHA256";

	context = lws_create_context(&info);
	log_d("[Main] context created.");

	if (context == NULL) {
		log_d("[Main] context is NULL.");
		return -1;
	}

	log_d("start....");
	int n = 0;
	while (n >= 0) {
		n = lws_service(context, 50);
	}
	lws_context_destroy(context);

	return EXIT_SUCCESS;
}
