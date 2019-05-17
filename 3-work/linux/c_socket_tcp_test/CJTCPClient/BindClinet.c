/*
 * BindClinet.c
 *
 *  Created on: 2017年11月17日
 *      Author: cj
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>

#include "BindClient.h"

#define SOCKET_ERROR -1
#define INVALID_SOCKET  SOCKET_ERROR

#define log_d(format,...)  do { \
		printf("%s,%s,%d:"format"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  fflush(stdout); } while(0);

typedef struct {
    sem_t *sem;
    char name[10];
} os_signal;



typedef struct {
	int fd_sock;

	unsigned int connected :1; /**< whether it is currently connected */

	pthread_mutex_t _lock;
	os_signal *sem_close;
    
	pthread_t pt_recvdata;

	BindClient_connectionLost *_pfun_connlost;
	BindClient_messageArrived *_pfun_messArrived;
	void* context;
} BindClients;

int os_signal_create(os_signal **signal){
    os_signal *s=NULL;
    s=(os_signal*)malloc(sizeof(os_signal));
    if(s){
        s->sem=sem_open("signal", O_CREAT,S_IRUSR|S_IWUSR, 0);
        *signal=s;
    }
    return 0;
}

int os_signal_destroy(os_signal *signal){
    if(signal){
        sem_close(signal->sem);
        free(signal);
    }
    return 0;
}

int os_signal_wait(os_signal *sig){
    return sem_wait(sig->sem);
}

int os_signal_post(os_signal *sig){
    return sem_post(sig->sem);
}

static void *_t_bindclient(void *args);

int BindClient_create(BindClient *client) {
	BindClients *m = NULL;
	int rc = 0;
	m = (BindClients*) malloc(sizeof(BindClients));

	if (m == NULL) {
		rc = BINDCLIENT_FAILURE;
		goto exit;
	}
	memset(m, 0, sizeof(BindClients));
	pthread_mutex_init(&m->_lock, NULL);
    os_signal_create(&m->sem_close);

	*client = m;
	exit: return rc;
}

int BindClient_setCallbacks(BindClient handle, void* context,
		BindClient_connectionLost* cl, BindClient_messageArrived* ma) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;
	if (m == NULL) {
		rc = BINDCLIENT_FAILURE;
		goto exit;
	}
	m->_pfun_connlost = cl;
	m->_pfun_messArrived = ma;
	m->context = context;
	exit: return rc;
}

int BindClient_connect(BindClient handle, const char *ip, unsigned short port) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;
	int nsd;
	struct sockaddr_in addr;

	pthread_attr_t attr;

	if (m == NULL) {
		rc = BINDCLIENT_FAILURE;
		goto exit;
	}

	pthread_mutex_lock(&m->_lock);

	if (m->fd_sock > 0) {
		rc = -1;
		goto exitclient;
	}

	m->connected = 0;

	nsd = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == nsd) {
		rc = errno;
		close(nsd);
		goto exitclient;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	if (connect(nsd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0) {
		printf("connect error %s \n", strerror(errno));
		close(nsd);
		rc = errno;
		goto exitclient;
	}

#ifdef SO_NOSIGPIPE
    int opt = 1;
    setsockopt(nsd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&opt, sizeof(opt));
#endif
    
	m->fd_sock = nsd;
	m->connected = 1;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&m->pt_recvdata, NULL, _t_bindclient, m); //bug...
	usleep(100);
	pthread_attr_destroy(&attr);
	exitclient: pthread_mutex_unlock(&m->_lock);
	exit: return rc;
}

int BindClient_isConnected(BindClient handle) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;
	if (m) {
		pthread_mutex_lock(&m->_lock);
		rc = m->connected;
		pthread_mutex_unlock(&m->_lock);
	}
	return rc;
}

static int BindClient_calc2buflen(unsigned char* buf, size_t len) {
	int siz = 0;

	do {
		char d = len % 128;
		len /= 128; /* if there are more digits to encode, set the top bit of this digit */

		if (len > 0)
			d |= 0x80;
		buf[siz++] = d;
	} while (len > 0);
	return siz;
}

static size_t sock_send(int fd, unsigned char *data, size_t dlen) {
	ssize_t ret;
	size_t slen = 0;

	while (1) {
		ret = send(fd, data + slen, dlen - slen, 0);

		if (ret == SOCKET_ERROR) {
			int err = errno;
			if (err == EWOULDBLOCK || err == EAGAIN) {
				continue;
			}
			return ret;
		}
		slen += ret;
		if (slen >= dlen)
			break;
	}
	return slen;
}

int BindClient_send(BindClient handle, unsigned char type, void *data, size_t dlen) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;

	unsigned char head[10];
	//unsigned char *buff = NULL;
	//int buflen = 0;
	int dlenbytes = 0;
	unsigned char crc1 = 0;
	int i = 0;
	if (m == NULL) {
		rc = BINDCLIENT_FAILURE;
		goto exit;
	}
	//分别表示(每个字节的低 7 位用于编码数据,最高位是标志位): 参考MQTT协议文档
	//1 个字节时,从 0(0x00)到 127(0x7f)
	//2 个字节时,从 128(0x80,0x01)到 16383(0Xff,0x7f)
	//3 个字节时,从 16384(0x80,0x80,0x01)到 2097151(0xFF,0xFF,0x7F)
	//4 个字节时,从 2097152(0x80,0x80,0x80,0x01)到 268435455(0xFF,0xFF,0xFF,0x7F)
#if 0
//	head[0] = 0xEE;
//	head[1] = type;
//	dlenbytes = BindClient_calc2buflen(&head[2], dlen);
//
//	for (i = 0; i < dlenbytes + 2; i++) {
//		crc1 += head[i];
//	}
//	for (i = 0; i < dlen; i++)
//		crc1 += ((unsigned char *) data)[i];
//
//	crc1 = ~crc1 + 1;
//	pthread_mutex_lock(&m->_lock);
//
//	if (sock_send(m->fd_sock, head, dlenbytes + 2) > 0) {
//		if (sock_send(m->fd_sock, data, dlen) > 0) {
//			sock_send(m->fd_sock, &crc1, 1);
//		}
//	}
#else
    
    head[0] = 0xEE;
    dlenbytes = BindClient_calc2buflen(&head[1], dlen);
    
 
    pthread_mutex_lock(&m->_lock);
    
    if (sock_send(m->fd_sock, head, dlenbytes + 1) > 0) {
        if (sock_send(m->fd_sock, data, dlen) > 0) {
            
        }
    }

#endif

//	while (1) {
//		ret = send(m->fd_sock, data + slen, dlen - slen, 0);
//
//		if (ret == SOCKET_ERROR) {
//			int err = errno;
//			if (err == EWOULDBLOCK || err == EAGAIN) {
//				continue;
//			}
//			break;
//		}
//		slen += ret;
//		if (slen >= dlen)
//			break;
//	}

	pthread_mutex_unlock(&m->_lock);
	exit: return rc;
}

static ssize_t socket_close(int socket) {
	ssize_t rc;

#if defined(WIN32) || defined(WIN64)
	if (shutdown(socket, SD_BOTH) == SOCKET_ERROR)
	Socket_error("shutdown", socket);
	if ((rc = closesocket(socket)) == SOCKET_ERROR)
	Socket_error("close", socket);
#else
	if (shutdown(socket, SHUT_WR) == SOCKET_ERROR) {
//Socket_error("shutdown", socket);
	}
	if ((rc = recv(socket, NULL, (size_t) 0, 0)) == SOCKET_ERROR) {
///Socket_error("shutdown", socket);
	}
	if ((rc = close(socket)) == SOCKET_ERROR) {
//Socket_error("close", socket);
	}
#endif
	return rc;
}

static int BindClient_close1(BindClient handle, int timeout,
		int call_connection_lost, int stop) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;
	pthread_mutex_lock(&m->_lock);

	if (m->fd_sock > 0) {
		socket_close(m->fd_sock);
		m->fd_sock = -1;
		m->connected = 0;

		if (call_connection_lost && m->_pfun_connlost) {
			(*(m->_pfun_connlost))(m->context, NULL);
		}
	} else {
		rc = BINDCLIENT_FAILURE;
	}
	pthread_mutex_unlock(&m->_lock);
	return rc;
}

int BindClient_close(BindClient handle) {
	BindClients *m = (BindClients *) handle;
	int rc = 0;
	if (m == NULL) {
		rc = BINDCLIENT_FAILURE;
		goto exit;
	}

	rc = BindClient_close1(handle, 0, 0, 1);
	log_d("close...");
	if (rc == 0)
		os_signal_wait(m->sem_close);
//pthread_join(m->pt_recvdata, NULL);
	exit: return rc;
}

void BindClient_destroy(BindClient *handle) {
	BindClients *m = (BindClients *) handle;
	if (m) {
		pthread_mutex_destroy(&m->_lock);
		os_signal_destroy(m->sem_close);
		free(m);
	}
}

static ssize_t sock_gets(int fd, unsigned char *c, int len) {
	ssize_t rc = 0;
	int err = 0;
	int rlen = 0;
	while (1) {
		if ((rc = recv(fd, c + rlen, len - rlen, 0)) == SOCKET_ERROR) {
			err = errno;
			if (err == EWOULDBLOCK || err == EAGAIN) {
				continue;
			}
			break;
		}
		rlen += rc;
		if (rlen >= len)
			break;
	}
	return rc;
}

static ssize_t BindClient_caclfrombuff(int fd, int *value, unsigned char *crc) {
	int len = 0;
	int multiplier = 1;
	ssize_t rc = 0;
	unsigned char c = 0;

#define MAX_LENGTH_BYTES 4
	do {
		if (++len > MAX_LENGTH_BYTES)
			break;
		//c = buff[i++];
		rc = sock_gets(fd, &c, 1);
		if (rc <= 0)
			break;
		*crc += c;
		*value += (c & 127) * multiplier;
		multiplier *= 128;
	} while ((c & 128) != 0);
	return rc;
}

static void *_t_bindclient(void *args) {
#define CACHES_SIZE (5)

	BindClients *m = (BindClients *) args;
	unsigned char buf[CACHES_SIZE];

	ssize_t rc = 0;
	int remlength = 0;
	unsigned char *data = NULL;
	unsigned char crc1 = 0;

	memset(buf, 0, sizeof(buf));

	//log_d("recv start");
	while (1) {
		crc1 = 0;
		rc = sock_gets(m->fd_sock, buf, 2);

		if (rc <= 0) {
			break;
		}
		crc1 += buf[0];
		crc1 += buf[1];

		remlength = 0;
		rc = BindClient_caclfrombuff(m->fd_sock, &remlength, &crc1);

		if (remlength > 0) {
			data = (unsigned char*) malloc(remlength);
			rc = sock_gets(m->fd_sock, data, remlength);

			if (rc <= 0) {
				break;
			}
		}
		rc = sock_gets(m->fd_sock, buf, 1);
		if (rc <= 0) {
			break;
		}
		crc1 = ~crc1 + 1;
		if (crc1 == buf[0]) {
			if (m->_pfun_messArrived) {
				(*(m->_pfun_messArrived))(m->context, buf[1], data, remlength);
			}
        } else {
            log_d("crc-err:");
            free(data);
        }
	}
	log_d("exit recv");
	if (BindClient_close1(m, 0, 1, 1) != 0) {
		log_d("post..");
		os_signal_post(m->sem_close);
	}
	return NULL;
}
