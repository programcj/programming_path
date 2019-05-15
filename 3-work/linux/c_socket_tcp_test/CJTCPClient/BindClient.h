/*
 * BindClient.h
 *
 *  Created on: 2017年11月17日
 *      Author: cj
 */

#ifndef BINDCLIENT_H_
#define BINDCLIENT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define BINDCLIENT_OK				0
#define BINDCLIENT_FAILURE  -1

typedef void * BindClient;
typedef void BindClient_connectionLost(void* context, char* cause);
typedef int BindClient_messageArrived(void* context, int type, void *data,
		int dlen);

int BindClient_create(BindClient *handle);

int BindClient_setCallbacks(BindClient handle, void* context,
		BindClient_connectionLost* cl, BindClient_messageArrived* ma);

int BindClient_connect(BindClient handle, const char *ip, unsigned short port);

int BindClient_isConnected(BindClient handle);

int BindClient_send(BindClient handle, unsigned char type, void *data, size_t dlen);

int BindClient_close(BindClient handle);

void BindClient_destroy(BindClient *handle);

#if defined(__cplusplus)
}
#endif

#endif /* BINDCLIENT_H_ */
