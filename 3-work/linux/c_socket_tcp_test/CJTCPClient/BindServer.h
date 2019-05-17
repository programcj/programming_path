/*
 * BindServer.h
 *
 *  Created on: 2017年11月17日
 *      Author: cj
 */

#ifndef BINDSERVER_H_
#define BINDSERVER_H_

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

typedef void * BindServer;

int BindServer_create(BindServer *handle);

int BindServer_bind(BindServer handle, const char *ip, int port);

int BindServer_loop(BindServer handle);

int BindServer_close(BindServer handle);

int BindServer_destroy(BindServer handle);

#if defined(__cplusplus)
}
#endif

#endif /* BINDSERVER_H_ */
