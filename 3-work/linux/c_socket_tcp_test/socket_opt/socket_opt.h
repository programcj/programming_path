/*
 * socket_opt.h
 *
 *  Created on: 2019年4月5日
 *      Author: cj
 */

#ifndef _SOCKET_OPT_
#define _SOCKET_OPT_

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

//获取输入输出缓冲已经缓冲的数据大小
#include <linux/sockios.h>
#include <sys/ioctl.h>

#define socket_get_count_out_queue(fd,pending)	ioctl(fd, SIOCOUTQ, pending)
#define socket_get_count_in_queue(fd,pending)	ioctl(fd, SIOCINQ, pending)

#define socket_get_error(fd) \
	({ int v=0; socklen_t len=sizeof(v); int ret=getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&v, &len); ret==-1?ret:v; })

#define socket_get_sendbuf_size(fd) \
	({ int v=0; socklen_t len=sizeof(v); int ret=getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&v, &len); ret==-1?ret:v; })

#define socket_get_rcvbuf_size(fd) 	\
	({ int v=0; socklen_t len=sizeof(v); int ret= getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&v, &len); ret==-1?ret:v; })

static inline int socket_nosigpipe(int fd, int flag) {
#ifdef SO_NOSIGPIPE
	return setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &flag, sizeof flag);
#else
	return 0;
#endif
}

static inline int socket_getopt(int fd, int opt, int *v) {
	socklen_t vlen = sizeof(int);
	return getsockopt(fd, SOL_SOCKET, opt, &v, &vlen);
}

static inline int socket_get_recvbufforce(int fd) {
	socklen_t vlen = sizeof(int);
	int v;
	int ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &v, &vlen);
	if (ret == -1)
		return -1;
	return v;
}

static inline int socket_get_sendbufforce(int fd) {
	socklen_t vlen = sizeof(int);
	int v;
	int ret = getsockopt(fd, SOL_SOCKET, SO_SNDBUFFORCE, &v, &vlen);
	if (ret == -1)
		return -1;
	return v;
}

static inline int socket_setoptint(int fd, int leve, int opt, int v) {
	return setsockopt(fd, leve, opt, (char*) &v, sizeof v);
}

#define socket_setopt(fd, opt, v) socket_setoptint(fd,SOL_SOCKET,opt,v)

//cat /proc/sys/net/ipv4/tcp_wmem
//cat /proc/sys/net/core/wmem_max
//SO_SNDBUFFORCE 可以重写wmem_max限制
#define socket_set_sendbuf_size(fd, size) \
	({ int v=size; setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*) &v, sizeof v); })

//cat /proc/sys/net/core/rmem_max
//SO_RCVBUFFORCE 可以重写rmem_max限制
#define socket_set_rcvdbuf_size(fd,size) \
	({ int v=size; setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*) &v, sizeof v); })

#define socket_set_reuseport(fd, flag) \
		({ int v=flag; setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (void*) &v, sizeof v); })

#define socket_set_reuseaddr(fd, flag) \
		({ int v=flag; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof v); })

//广播
#define socket_set_broadcast(fd, flag) \
	({ int v=flag; setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*) &v, sizeof v); })

#define socket_send_nosignal(fd, buff)   send(fd, buff, strlen(buff), MSG_NOSIGNAL)

/**
 net.ipv4.tcp_keepalive_intvl = 20
 net.ipv4.tcp_keepalive_probes = 3
 net.ipv4.tcp_keepalive_time = 60
 */
//int keepAlive = 1; // 开启keepalive属性
//int keepIdle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
//int keepInterval = 5; // 探测时发包的时间间隔为5 秒
//int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
//
#define socket_set_keepalive(fd, flag) 	 socket_setoptint(fd,SOL_SOCKET,SO_KEEPALIVE, flag)
#define socket_set_tcp_keepidle(fd, v)   socket_setoptint(fd,IPPROTO_TCP,TCP_KEEPIDLE,v)
#define socket_set_tcp_keepintvl(fd, v)  socket_setoptint(fd,IPPROTO_TCP,TCP_KEEPINTVL,v)
#define socket_set_tcp_keepcnt(fd, v)  	 socket_setoptint(fd,IPPROTO_TCP,TCP_KEEPCNT,v)
#define socket_set_tcp_nodelay(fd, flag) socket_setoptint(fd,IPPROTO_TCP,TCP_NODELAY,flag)
#define socket_set_ipv6only(fd, flag)	 socket_setoptint(fd, IPPROTO_IPV6, IPV6_V6ONLY, flag)

//https://blog.csdn.net/qiaotokong/article/details/25560797
static inline int socket_is_established(int fd) {
	struct tcp_info info;
	socklen_t len = sizeof(struct tcp_info);

	if (getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &len) < 0) {
		return -1; //err
	}
	return info.tcpi_state == TCP_ESTABLISHED;
}

//非阻塞模式
static inline int socket_nonblock(int fd, int flag) {
	int opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1) {
		return -1;
	}
	if (flag)
		opt |= O_NONBLOCK;
	else
		opt &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, opt) == -1) {
		return -1;
	}
	return 0;
}

#include <sys/socket.h>
#include <arpa/inet.h>

//inet_aton
//inet_ntoa
//--
//inet_pton
//inet_ntop
//--
//获取socket 本地地址
static inline int socket_getsockname(int fd, void *addr, socklen_t *len) {
	return getsockname(fd, (struct sockaddr*) addr, len);
}

//获取socket 远端地址
static inline int socket_get_peeraddrstring(int fd, char *buf, int len,
		in_port_t *port) {
	struct sockaddr_storage addr;
	socklen_t addrlen;
	addrlen = sizeof(addr);
	if (!getpeername(fd, (struct sockaddr *) &addr, &addrlen)) {
		if (addr.ss_family == AF_INET) {
			*port = ((struct sockaddr_in *) &addr)->sin_port;
			if (inet_ntop(AF_INET,
					&((struct sockaddr_in *) &addr)->sin_addr.s_addr, buf,
					len)) {
				return 0;
			}
		} else if (addr.ss_family == AF_INET6) {
			*port = ((struct sockaddr_in6 *) &addr)->sin6_port;
			if (inet_ntop(AF_INET6,
					&((struct sockaddr_in6 *) &addr)->sin6_addr.s6_addr, buf,
					len)) {
				return 0;
			}
		}
	}
	return -1;
}

#define socket_af_inet_stream() socket(AF_INET, SOCK_STREAM,0)
#define socket_af_inet_dgram() socket(AF_INET,SOCK_DGRAM,0)
#define socket_af_unix_stream() socket(AF_UNIX, SOC_STREAM, 0)

#include <sys/un.h>
#include <unistd.h>

static inline void socket_addr_in_set(struct sockaddr_in *addr, int family,
		unsigned short port, const char *ipstr) {
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	if (ipstr) {
		inet_pton(addr->sin_family, ipstr, &addr->sin_addr);
	} else
		addr->sin_addr.s_addr = INADDR_ANY;
}

static inline int socket_bind(int fd, void *addr, socklen_t len) {
	return bind(fd, (const struct sockaddr *) addr, len);
}

static inline int socket_connect(int fd, void *addr, socklen_t len) {
	return connect(fd, (struct sockaddr *) &addr, len);
}

static inline int socket_bind_af_unix(int fd, const char *path) {
	struct sockaddr_un addr;
	unlink(path);
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, path);
	return socket_bind(fd, &addr, sizeof addr);
}

//接收指定长度的数据到buf中
#if 0
int socket_readn(int fd, void *buf, int len) {
	int rt = 0;
	int index = 0;
	while (len > 0) {
		rt = recv(fd, buf + index, len, 0);
		if (rt < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			continue;
			return -1;
		}
		if (rt == 0)
		return -1;
		index += rt;
		len -= rt;
	}
	return index;
}

//return  <01, close> <0, Need Try again>
static inline int socket_recvex(int fd, void *buf, int maxlen) {
	int rc = 0;
	rc = recv(fd, buf, maxlen, 0);
	if (rc < 0) {
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
		return 0;
		return -1;
	}
	if (rc == 0)
	return -1;
	return rc;
}
#endif

#include <netdb.h>

static inline void net_print_hostnameip(const char *hostname) {
	struct hostent *host;
	host = gethostbyname2(hostname, AF_INET);
	if (host) {
		printf("hostname:[%s]:[%s] %d \n", hostname, host->h_name,
				host->h_length);
		for (int i = 0; host->h_addr_list[i]; i++) {
			//inet_ntop(AF_INET, host->h_addr_list[i], buf, sizeof(buf));
			printf("%s\n", inet_ntoa(*(struct in_addr *) host->h_addr_list[i]));
		}
	}
}

static inline void net_print_addinfoip(const char *hostname) {
	struct addrinfo hints;
	struct addrinfo *res, *cur;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; /* Allow IPv4 */
	hints.ai_flags = AI_PASSIVE;/* For wildcard IP address */
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_socktype = SOCK_STREAM;

	int ret = getaddrinfo(hostname, NULL, &hints, &res);
	if (ret == 0) {
		for (cur = res; cur != NULL; cur = cur->ai_next) {
			printf("%s\n",
					inet_ntoa(((struct sockaddr_in *) cur->ai_addr)->sin_addr));
		}
		freeaddrinfo(res);
	}
}

#include <sys/stat.h>

static inline int io_mkfifo_create(const char *name, int mode) {
	unlink(name);
	return mkfifo(name, mode);
}

#define io_pipe_rw(fd)  pipe(fd)
#include <sys/msg.h>
#include <sys/ipc.h>
#define io_msg_type_def(type, len) struct type { long int msgtype; unsigned char data[len]; }
#define io_msgget_id(key, msgflg) 					msgget (key, msgflg)
#define io_msgsnd(id, msg, msgsiz,msgflg)  			msgsnd(id, (const void *)msg, msgsiz, msgflg)
#define io_msgrcv(id, msg, msgsz, msgtype,msgflg) 	msgrcv(id, (void *)msg, msgsz, msgtype,msgflg)
#define io_msgrecv_nowait(id,msg, msgsiz,msgtype)	msgrcv(id,(void*)msg, msgz, msgtype, IPC_NOWAIT)
#define io_msgrm_id(msgid)							msgctl(msgid, IPC_RMID, 0)

#include <sys/shm.h>
#include <sys/sem.h>

#define io_shmget_id					shmget
#define io_shmat_ptr(shmid,addr,flg)	shmat(shmid,addr,flg)
#define io_shm_entry(shmptr, type) 		(type)(shmptr)
#define io_shmdt_ptr(shmptr)			shmdt(shmptr)
#define io_shm_rmid(shmid)				shmctl(shmid, IPC_RMID, 0)
#define io_mmap	mmap

/**
 int shmid=shmget((key_t)1234, 100, 0666|IPC_CREATE);
 void *shm=shmat(shmid, 0, 0);
 while(1) strcpy(shm,"hello");
 shmdt(shm);
 shmctl(shmid, IPC_RMID,0);
 --other thread--
 while(1) printf("%s\n", shm);
 */
#include <sys/mman.h>

#include <fcntl.h>

#define io_open(name, flag, ...)	open(name, flag, ##__VA_ARGS__)
/**
 * $man open
 * flags:
 * O_RDONLY O_WRONLY O_RDWR
 *
 * O_CREAT
 * O_EXCL
 * O_NOCTTY
 * O_TRUNC
 * O_APPEND
 *
 * sample: O_RDWR|O_CREAT|O_TRUNC
 */
#endif /* _SOCKET_OPT_ */
