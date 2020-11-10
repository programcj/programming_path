/*
 * socket_forward.c
 *
 *  Created on: 2020年5月7日
 *      Author: cc
 *
 *  socket 转发
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <sys/epoll.h>
#include <sys/poll.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>

#include "tools/log.h"
#include "list.h"

///////// sock_stream //////
#define DATABUFF_SIZE (1024 * 500) //64K , UDP 协议内容长度为2字节
struct databuff {
	int size;
	int pos;
	int pread;
	uint8_t data[DATABUFF_SIZE];
};

struct timer {
	uint64_t start;
	uint64_t interval;
};

enum session_types {
	SESSION_TYPE_NONE = 0,
	SESSION_TYPE_HTTP_REQ_GET,
	SESSION_TYPE_HTTP_REQ_POST,
	SESSION_TYPE_HTTP_REQ_HEAD,
	SESSION_TYPE_HTTP_REQ_PUT,
	SESSION_TYPE_HTTP_REQ_DELETE,
	SESSION_TYPE_HTTP_REQ_OPTIONS,
	SESSION_TYPE_HTTP_REQ_TRACE,
	SESSION_TYPE_HTTP_REQ_CONNECT,
	SESSION_TYPE_HTTP_REQ_PATCH,
	SESSION_TYPE_HTTP_REQUEST, //
	SESSION_TYPE_HTTP_RESPONSE, //
	SESSION_TYPE_HTTP_REQ_ONVIF, //
	SESSION_TYPE_HTTP_REQ_NVR_nuiview, //
	SESSION_TYPE_HTTP_RES_ONVIF,
	SESSION_TYPE_HTTP_RES_NVR_nuiview,
	SESSION_TYPE_SIP, //
};

struct sock_stream {
	struct list_head list;
	int proto; //IPPROTO_TCP  IPPROTO_UDP
	int fd;  //如果是UDP，则此fd无效
	struct sockaddr_in addrremote;  //远端
	struct sockaddr_in addrlocal;   //本地

	char remoteip[30]; //
	int remoteport;

	char localip[30];
	int localport;

	char description[100];
	enum session_types stype; //会话类型 HTTP

	int tm_lastr; //最后接收时间
	int tm_lastw; //最后发送时间
	int isfirst;  //首次接收情况
	int isclose;  //是否关闭
	struct timer timeoutms; //超时时间

	void *puser; //User data variable
	struct databuff buff; //一般为接收buff
};

/////////// forward_server
struct forward_session;

struct sub_filter {
	struct sub_filter *next;
	char str1[50]; //IP+port
	char str2[50];
};

void sub_filter_free(struct sub_filter *head) {
	struct sub_filter *lastnext = head;

	while (head != NULL) {
		lastnext = head->next;
		free(head);
		head = lastnext;
	}
}

struct sub_filter *sub_filter_add(struct sub_filter *head, const char *str1,
		const char *str2) {
	struct sub_filter *v = (struct sub_filter *) calloc(
			sizeof(struct sub_filter), 1);
	struct sub_filter *lastnext = NULL;
	if (v) {
		strncpy(v->str1, str1, 50);
		strncpy(v->str2, str2, 50);
	}
	if (head == NULL) {
		v->next = NULL;
		head = v;
	} else {
		lastnext = head;
		while (lastnext->next != NULL)
			lastnext = lastnext->next;
		lastnext->next = v;
		v->next = NULL;
	}
	return head;
}

//转发源IP， 转发服务IP， 转发出IP，转发目的IP
//fsrc_ip    fsrc_port
//fserver_ip fserver_port
//fout_ip    fout_port
//fdst_ip    fdst_port
struct forward_server {
	struct list_head list;
	int proto; //协议（TCP/UDP）
	char server_name[20]; //http/ onvif(http) onvif_discover_Probe(udp)  sip(udp/tcp)

	//转发服务IP，需要监听
	char f_server_ip[20]; //转发服务IP
	int f_server_port; //转发服务端口

	char f_out_ip[20]; //转发出IP,端口未知

	char f_dst_ip[20]; //转发目的IP
	int f_dst_port;    //转发目的端口

	int (*forward_handle)(struct forward_session *sess,
			struct sock_stream *streamin, struct sock_stream *streamout); //转发数据处理函数

	struct sub_filter *sub_filter_forwardfromsrc; //替换报文 转发源
	struct sub_filter *sub_filter_forwardfromdst; //替换报文 转发目标
};

struct forward_session { //转发会话
	struct list_head list;
	struct sock_stream sforward_src; //转发发起源
	struct sock_stream sforward_dst; //转发到目标
	struct forward_server *forward_ser; //属于那个转发服务
};

struct socket_server {
	struct list_head list;
	int fd;
	struct sockaddr_in addr;
	struct forward_server *forward_ser; //此服务属于那个转发服务
};

//#define log_d(fmt, ...) do{ \
//	 struct timespec tm;   \
//	 clock_gettime(CLOCK_MONOTONIC, &tm); \
//	 uint64_t t = tm.tv_sec * 1000 + tm.tv_nsec / 1000000; \
//	 printf("(%llu, %s,%s,%d)"fmt, t, __FILE__, __FUNCTION__, __LINE__,## __VA_ARGS__); \
//  } while(0)

void sockaddr_to_ipinfo(struct sockaddr_in *addr, char *ipstr, int *port) {
	inet_ntop(AF_INET, &addr->sin_addr, ipstr, 20);
	*port = ntohs(addr->sin_port);
}

int socket_send(int fd, const void *data, int size) {
	int ret = 0;
	int pos = 0;

	while (size > 0) {
		ret = send(fd, data + pos, size, 0);

		if (ret <= 0) { //中断，需要重新发送
			if (ret < 0
					&& (errno == EAGAIN || errno == EWOULDBLOCK
							|| errno == EINTR)) {
				usleep(1000);
				continue;
			}
			return -1;
		}
		if (ret > 0) {
			size -= ret;
			pos += ret;
		}
	}
	return pos;
}

int socket_recv(int fd, void *data, int size) {
	int ret;
	int rlen = 0;

	while (size) {
		ret = recv(fd, (uint8_t *) data + rlen, size, 0);
		if (ret == 0)
			return -1;
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		}
		size -= ret;
		rlen += ret;
	}
	return rlen;
}

int socket_connect(const char *ip, int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int ret;
	if (fd == -1)
		return fd;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);

	ret = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1)
		goto _reterr;

	return fd;
	_reterr: if (fd != -1)
		close(fd);
	return -1;
}

int socket_server_create_tcp(const char *ip, int port) {
	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int ret;
	if (fd < 0)
		return -1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
//in.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, ip, &addr.sin_addr);
	int reuseport = 1;
	int reuseaddr = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1)
		goto _reterr;

	ret = listen(fd, 10);
	if (ret == -1)
		goto _reterr;

	return fd;
	_reterr: if (fd != -1)
		close(fd);
	return -1;
}

#define strncasecmpex(s1, s2) strncasecmp(s1, s2, strlen(s2))
#define strncmpex(s1, s2) strncmp(s1, s2, strlen(s2))

void strcpy_ipv4(char *dst, const char *src) {
	const char *ptr = src;
	int point = 0;
	while (ptr - src <= 15) {
		if (*ptr == '.')
			point++;

		if (point <= 3 && (isdigit(*ptr) || *ptr == '.'))
			*dst++ = *ptr++;
		else
			break;
	}
	*dst++ = 0;
}

char *string_line_endptr(const char *ptr) {
	while (*ptr) {
		if (*ptr == '\r')
			break;
		if (*ptr == '\n')
			break;
		ptr++;
	}
	return (char*) ptr;
}

char *string_line_end(const char *ptr, int *linesize, char **ptrend) {
	char *ln = strchr(ptr, '\n');
	*linesize = 0;
	if (ln) {
		ln++;
		*linesize = ln - ptr;
		if (ptrend)
			*ptrend = ln;
	} else {
		*linesize = strlen(ptr);
		if (ptrend)
			*ptrend = (char*) ptr + *linesize;
	}
	return (char*) ptr;
}

char *string_httphead_replace(const char *src, int slen, int *dstlen,
		const char *name, const char *fmt, ...) {
	const char *ptr = src;
	const char *ptr_end = src + slen;
	const char *ptrlen;

	char *tmpbuf;
	char tv = 0;
	size_t tmpbufsize = 0;

	va_list v;
	va_start(v, fmt);

	FILE *fp = open_memstream(&tmpbuf, &tmpbufsize);

	//fwrite(ptr, ptr_end - ptr, 1, fp);
	while (ptr < ptr_end) {
		ptrlen = strchr(ptr, '\n');

		if (strncasecmpex(ptr, name) == 0) {
			fprintf(fp, "%s:", name);
			vfprintf(fp, fmt, v);
			fprintf(fp, "\r\n");
			ptr = ptrlen + 1;
			continue;
		}
		fwrite(ptr, ptrlen - ptr + 1, 1, fp);
		ptr = ptrlen + 1;
	}
	fclose(fp);
	va_end(v);
	*dstlen = tmpbufsize;
	return tmpbuf;
}

char *string_replace2(const char *src, int slen, int *dstlen,
		struct sub_filter *filter) {
	char *tmpbuf;
	size_t tmpbufsize = 0;

	const char *ptr = src;
	const char *ptr_end = src + slen;

	const struct sub_filter *subfilter_next;

	FILE *fp = open_memstream(&tmpbuf, &tmpbufsize);
	if (fp) {
		int i = 0;
		int flag = 0;
		while (ptr < ptr_end) {
			flag = 0;
			for (subfilter_next = filter; subfilter_next != NULL;
					subfilter_next = subfilter_next->next) {
				if (strlen(subfilter_next->str1) > 0
						&& memcmp(ptr, subfilter_next->str1,
								strlen(subfilter_next->str1)) == 0) {
					fwrite(subfilter_next->str2, strlen(subfilter_next->str2),
							1, fp);
					ptr += strlen(subfilter_next->str1);
					flag = 1;
				}
			}
			if (flag)
				continue;
			fwrite(ptr, 1, 1, fp);
			ptr++;
		}
		fclose(fp);
	}
	*dstlen = tmpbufsize;
	return tmpbuf;
}

int string_replace(const char *src, int slen, char *dst, int *dstlen,
		struct sub_filter *filter) {
	char **tmpbuf;
	int tmpbufsize = 0;

	const char *ptr = src;
	const char *ptr_end = src + slen;
	char *ptrto = dst;
//	FILE *fp = open_memstream(&tmpbuf, &tmpbufsize);
//	if (fp) {
	int flag = 0;
	struct sub_filter *subfilter_next;

	while (ptr < ptr_end) {
		flag = 0;
		for (subfilter_next = filter; subfilter_next != NULL; subfilter_next =
				subfilter_next->next) {
			if (strlen(subfilter_next->str1) > 0
					&& memcmp(ptr, subfilter_next->str1,
							strlen(subfilter_next->str1)) == 0) {
				memcpy(ptrto, subfilter_next->str2,
						strlen(subfilter_next->str2));
				ptrto += strlen(subfilter_next->str2);
				ptr += strlen(subfilter_next->str1);
				flag = 1;
			}
		}
		if (flag)
			continue;
		*ptrto++ = *ptr++;
	}
	*dstlen = ptrto - dst;
	return 0;
}

//查找http头长度 -1 长度未找到
int string_http_heard_getlength(const char *buf, int len) {
	const char *end = buf + len;
	const char *ptr = buf;

	while (ptr < end) {
		if (ptr + 1 < end && *ptr == '\n' && *(ptr + 1) == '\n') {
			return ptr - buf + 2;
		}
		if (ptr + 2 < end && *ptr == '\n' && *(ptr + 1) == '\r'
				&& *(ptr + 2) == '\n') {
			return ptr - buf + 3;
		}
		if (ptr + 2 < end && *ptr == '\r' && *(ptr + 1) == '\n'
				&& *(ptr + 2) == '\n') {
			return ptr - buf + 3;
		}
		if (ptr + 3 < end && *ptr == '\r' && *(ptr + 1) == '\n'
				&& *(ptr + 2) == '\r' && *(ptr + 3) == '\n') {
			return ptr - buf + 4;
		}
		ptr++;
	}

	return -1;
}

int string_http_head_get_content_length(const char *buffhead, int headlen) {
	const char *ptr = buffhead;
	const char *ptr_end = buffhead + headlen;
	int content_length = -1;
	while (ptr < ptr_end) {
		if (0 == strncasecmpex(ptr, "Content-Length:")) {
			content_length = atoi(ptr + strlen("Content-Length:"));
		}
		ptr = strchr(ptr, '\n');
		if (!ptr)
			break;
		if (ptr + 2 < ptr_end) {
			if (ptr[1] == '\r' && ptr[2] == '\n')
				break;
		}
		ptr++;
	}
	return content_length;
}

int buff_parse_ishttpresponse(char *buff, int len) {
	if (strncasecmp(buff, "HTTP/", 5) == 0) {
		return 1;
	}
	return 0;
}

int buff_parse_http_req_gettype(const char *buff) {
	if (strncmpex(buff, "GET ") == 0)
		return SESSION_TYPE_HTTP_REQ_GET;
	if (strncmpex(buff, "POST ") == 0)
		return SESSION_TYPE_HTTP_REQ_POST;
	if (strncmpex(buff, "HEAD ") == 0)
		return SESSION_TYPE_HTTP_REQ_HEAD;
	if (strncmpex(buff, "PUT ") == 0)
		return SESSION_TYPE_HTTP_REQ_PUT;
	if (strncmpex(buff, "DELETE ") == 0)
		return SESSION_TYPE_HTTP_REQ_DELETE;
	if (strncmpex(buff, "OPTIONS ") == 0)
		return SESSION_TYPE_HTTP_REQ_OPTIONS;
	if (strncmpex(buff, "TRACE ") == 0)
		return SESSION_TYPE_HTTP_REQ_TRACE;
	if (strncmpex(buff, "CONNECT ") == 0)
		return SESSION_TYPE_HTTP_REQ_CONNECT;
	if (strncmpex(buff, "PATCH ") == 0)
		return SESSION_TYPE_HTTP_REQ_PATCH;
	return SESSION_TYPE_NONE;
}

int buff_parse_is_onvif_request(const char *buff, int len) {
	const char *ptr = buff;
	const char *ptrend = buff + len;
	ptr = strstr(buff, "/onvif/");
	if (ptr && ptr < ptrend) {
		return SESSION_TYPE_HTTP_REQ_ONVIF;
	}
	return SESSION_TYPE_NONE;
}
//宇视NVR请求
int buff_parse_is_nvr_nuiview_request(const char *buff, int len) {
	const char *ptr = buff;
	const char *ptrend = buff + len;
	ptr = strstr(buff, "/cgi-bin/main-cgi");
	if (ptr && ptr < ptrend) {
		return SESSION_TYPE_HTTP_REQ_NVR_nuiview;
	}
	return SESSION_TYPE_NONE;
}

int socket_bind(int style, const char *ip, int port, int reuseport,
		int reuseaddr) {
	int fd = socket(AF_INET, style, 0);
	int ret;
	if (fd == -1)
		return fd;
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (strlen(ip) > 0)
		inet_aton(ip, &addr.sin_addr);

	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret == -1)
		goto _reterr;
	if (port == 0) {
		socklen_t addrlen = sizeof(addr);
		getsockname(fd, (struct sockaddr*) &addr, &addrlen);
		log_d("forward", "port:%d\n", ntohs(addr.sin_port));
	}

	return fd;

	_reterr: if (fd != -1)
		close(fd);
	return -1;
}

int databuff_append(struct databuff *buff, const void *ptr, int size) {
	memcpy(buff->data + buff->pos, ptr, size);
	buff->pos += size;
	return buff->pos;
}
int databuff_append_str(struct databuff *buff, const char *str) {
	return databuff_append(buff, str, strlen(str));
}

void databuff_clear(struct databuff *buff) {
	buff->pos = 0;
}

static void databuff_out_hex(struct databuff *buff, FILE *stream, int maxlen) {
	uint8_t *ptr = buff->data;
	uint8_t *ptr_end = ptr + buff->pos;
	int v;
	int i = 0;
	int prvpos = 0;
	int hexlen = 0;
	if (maxlen > 0 && maxlen < buff->pos) {
		ptr_end = ptr + maxlen;
	}
	while (ptr < ptr_end) {
		fprintf(stream, "%02X ", *ptr);
		hexlen = (i + 1) % 16;

		if (hexlen == 0 || ptr + 1 >= ptr_end) {

			while (hexlen != 0 && hexlen++ < 16) {
				fprintf(stream, "   ");
			}
			fprintf(stream, "|");
			//fprintf(stream, "|[%2d,%2d]",  prvpos, i);
			while (prvpos <= i) {
				v = ptr[prvpos - i]; //
				if (v == '\n' || v == '\r')
					v = '.';
				fputc(v, stream);
				prvpos++;
			}
			fprintf(stream, "|\n");
			prvpos = i + 1;
		}
		i++;
		ptr++;
	}
}

int timer_expired(struct timer *t) {
	/* Note: Can not return diff >= t->interval so we add 1 to diff and return
	 t->interval < diff - required to avoid an internal error in mspgcc. */
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	uint64_t curr = tm.tv_sec * 1000 + tm.tv_nsec / 1000000;
	uint64_t diff = (curr - t->start) + 1;
	return t->interval < diff;
}

void timer_restart(struct timer *t) {
//	struct timeval tv;
//	gettimeofday(&tv, NULL);
//	t->start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	t->start = tm.tv_sec * 1000 + tm.tv_nsec / 1000000;
}

void timer_set(struct timer *t, int interval) {
	t->interval = interval;
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	t->start = tm.tv_sec * 1000 + tm.tv_nsec / 1000000;
}

struct forward_session *forward_session_new() {
	struct forward_session *v = (struct forward_session*) calloc(
			sizeof(struct forward_session), 1);
	if (v) {
		v->sforward_src.buff.size = DATABUFF_SIZE;
		v->sforward_dst.buff.size = DATABUFF_SIZE;
	}
	return v;
}

void forward_session_free(struct forward_session *fs) {
	if (fs) {
		free(fs);
	}
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

int sock_stream_send(struct sock_stream *stream, uint8_t *data, int size) {
	int ret;
	int pos = 0;

	if (stream->proto == IPPROTO_TCP) {
		while (size > 0) {
			ret = send(stream->fd, data + pos, size, 0);

			if (ret <= 0) {
				//中断，需要重新发送
				if (ret < 0
						&& (errno == EAGAIN || errno == EWOULDBLOCK
								|| errno == EINTR)) {
					usleep(1000);
					continue;
				}
				log_d("forward", "forward", "send err[%s]\n",
						stream->description);
				return -1;
			}
			if (ret > 0) {
				size -= ret;
				pos += ret;
			}
		}
		return pos;
	}

	if (stream->proto == IPPROTO_UDP) {
		int addrlen = sizeof(struct sockaddr_in);
		pos = sendto(stream->fd, data, size, 0,
				(struct sockaddr*) &stream->addrremote, addrlen);
	}
	return pos;
}

int sock_stream_recv(struct sock_stream *stream) {
	int ret;
	if (stream->buff.size - stream->buff.pos == 0)
		return 0;
	ret = recv(stream->fd, stream->buff.data + stream->buff.pos,
			stream->buff.size - stream->buff.pos, 0);
	if (ret <= 0) {
		if (ret < 0
				&& (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
				{
			return 0;
		}
		stream->isclose = 1;
		return -1;
	}
	stream->buff.pos += ret;
	return ret;
}

int sock_stream_set_localaddr(struct sock_stream *stream, const char *localip,
		int localport) {
	stream->addrlocal.sin_family = AF_INET;
	stream->addrlocal.sin_port = htons(localport);
	stream->addrlocal.sin_addr.s_addr = inet_addr(localip);

	inet_ntop(AF_INET, &stream->addrlocal.sin_addr, stream->localip, 20);
	stream->localport = ntohs(stream->addrlocal.sin_port);
	return 0;
}

int sock_stream_set_remoteaddr(struct sock_stream *stream, const char *remoteip,
		int remoteport) {
	stream->addrremote.sin_family = AF_INET;
	stream->addrremote.sin_port = htons(remoteport);
	stream->addrremote.sin_addr.s_addr = inet_addr(remoteip);

	inet_ntop(AF_INET, &stream->addrremote.sin_addr, stream->remoteip, 20);
	stream->remoteport = ntohs(stream->addrremote.sin_port);
	return 0;
}

void sock_stream_descript(struct sock_stream *stream, int toremote) {
	if (toremote)
		sprintf(stream->description, "fd:%d L:%s:%d->R:%s:%d", stream->fd,
				stream->localip, stream->localport, stream->remoteip,
				stream->remoteport);
	else
		sprintf(stream->description, "fd:%d R:%s:%d->L:%s:%d", stream->fd,
				stream->remoteip, stream->remoteport, stream->localip,
				stream->localport);
}

int sock_stream_bind_udp_toremote(struct sock_stream *stream,
		const char *localip, int localport, const char *dstip, int dstport) {
	int fd = socket_bind(SOCK_DGRAM, localip, localport, 0, 0);
	if (fd == -1)
		return -1;
	stream->proto = IPPROTO_UDP;
	stream->fd = fd;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	getsockname(fd, (struct sockaddr*) &stream->addrlocal, &addrlen);

	stream->addrremote.sin_family = AF_INET;
	stream->addrremote.sin_port = htons(dstport);
	stream->addrremote.sin_addr.s_addr = inet_addr(dstip);

	inet_ntop(AF_INET, &stream->addrlocal.sin_addr, stream->localip, 20);
	stream->localport = ntohs(stream->addrlocal.sin_port);

	inet_ntop(AF_INET, &stream->addrremote.sin_addr, stream->remoteip, 20);
	stream->remoteport = ntohs(stream->addrremote.sin_port);
	sock_stream_descript(stream, 1);
	return 0;
}

int sock_stream_connect(struct sock_stream *stream, const char *ipstr, int port) {
	int fd = socket_connect(ipstr, port); //连接到转发目的IP
	if (fd == -1) {
		return -1;
	}
	stream->fd = fd;
	int addrlen;

	stream->proto = IPPROTO_TCP;
	stream->fd = fd;
	addrlen = sizeof(struct sockaddr_in);
	getsockname(fd, (struct sockaddr *) &stream->addrlocal, &addrlen);
	addrlen = sizeof(struct sockaddr_in);
	getpeername(fd, (struct sockaddr *) &stream->addrremote, &addrlen);

	inet_ntop(AF_INET, &stream->addrlocal.sin_addr, stream->localip, 20);
	stream->localport = ntohs(stream->addrlocal.sin_port);

	inet_ntop(AF_INET, &stream->addrremote.sin_addr, stream->remoteip, 20);
	stream->remoteport = ntohs(stream->addrremote.sin_port);

	sock_stream_descript(stream, 1);
	return 0;
}

//@type: IPPROTO_TCP
int sock_stream_build_tcp(struct sock_stream *stream, int fd) {
	stream->fd = fd;
	stream->proto = IPPROTO_TCP; //

	int addrlen = sizeof(struct sockaddr_in);
	getsockname(fd, (struct sockaddr *) &stream->addrlocal, &addrlen);
	inet_ntop(AF_INET, &stream->addrlocal.sin_addr, stream->localip, 20);
	stream->localport = ntohs(stream->addrlocal.sin_port);

	addrlen = sizeof(struct sockaddr_in);
	getpeername(fd, (struct sockaddr *) &stream->addrremote, &addrlen);
	inet_ntop(AF_INET, &stream->addrremote.sin_addr, stream->remoteip, 20);
	stream->remoteport = ntohs(stream->addrremote.sin_port);

	sock_stream_descript(stream, 1);
	return 0;
}

int sock_stream_accept(struct sock_stream *stream, int fd) {
	struct sockaddr_in inaddr;
	int addrlen = sizeof(inaddr);
//struct sock_stream *stream;
	int fdin = accept(fd, (struct sockaddr *) &inaddr, &addrlen);
	if (fdin <= 0)
		return -1;

//stream = (struct sock_stream *) calloc(sizeof(struct sock_stream));
	stream->proto = IPPROTO_TCP;
	stream->fd = fdin;
	memcpy(&stream->addrremote, &inaddr, sizeof(inaddr));
	addrlen = sizeof(struct sockaddr_in);
	getsockname(fdin, (struct sockaddr *) &stream->addrlocal, &addrlen);

	inet_ntop(AF_INET, &stream->addrlocal.sin_addr, stream->localip, 20);
	stream->localport = ntohs(stream->addrlocal.sin_port);
	inet_ntop(AF_INET, &stream->addrremote.sin_addr, stream->remoteip, 20);
	stream->remoteport = ntohs(stream->addrremote.sin_port);

	sock_stream_descript(stream, 0);
	return 0;
}

struct socket_server *socket_server_build(struct forward_server *fserver) {
	struct socket_server *sserver = (struct socket_server *) calloc(
			sizeof(struct socket_server), 1);
	if (sserver == NULL)
		return NULL;
	sserver->addr.sin_family = AF_INET;
	sserver->addr.sin_port = htons(fserver->f_server_port);
	sserver->addr.sin_addr.s_addr = inet_addr(fserver->f_server_ip);
	sserver->forward_ser = fserver;

	if (fserver->proto == IPPROTO_TCP) {
		sserver->fd = socket_server_create_tcp(fserver->f_server_ip,
				fserver->f_server_port);
		return sserver;
	} else if (fserver->proto == IPPROTO_UDP) {
		sserver->fd = socket_bind(SOCK_DGRAM, fserver->f_server_ip,
				fserver->f_server_port, 1, 1);
		return sserver;
	}
	free(sserver);
	sserver = NULL;
	return sserver;
}

int forward_session_handle_udp(struct forward_session *sess,
		struct sock_stream *streamin, struct sock_stream *streamout) {
	return -1;
}

static struct sub_filter *_subfiter_sip_INVITE_content = NULL;

void sip_INVITE_content_subfilter(const char *str1, const char *str2) {
	_subfiter_sip_INVITE_content = sub_filter_add(_subfiter_sip_INVITE_content,
			str1, str2);
}

int forward_session_handle_sip(struct forward_session *sess,
		struct sock_stream *streamin, struct sock_stream *streamout) {
	int ret;
	log_d("forward", "recv(%s -> %s):%d\n", streamin->description,
			streamout->description, streamin->buff.pos);
	databuff_out_hex(&streamin->buff, stdout, 30);
	//需要判断是否是国标SIP协议
	struct sub_filter *sub_filter_head = NULL;

	char str1[50];
	char str2[50];

	sprintf(str1, "%s:%d", streamin->localip, streamin->localport);
	sprintf(str2, "%s:%d", streamout->remoteip, streamout->remoteport);
	sub_filter_head = sub_filter_add(NULL, str1, str2);

	sprintf(str1, "%s:%d", streamin->remoteip, streamin->remoteport);
	sprintf(str2, "%s:%d", streamout->localip, streamout->localport);
	sub_filter_head = sub_filter_add(sub_filter_head, str1, str2);

	sprintf(str1, "%s:%d", streamin->remoteip, streamin->remoteport);
	sprintf(str2, "%s:%d", streamout->localip, streamout->localport);
	sub_filter_head = sub_filter_add(sub_filter_head, str1, str2);

	sprintf(str1, "%s", streamin->remoteip); //输入-远程IP
	sprintf(str2, "%s", streamout->localip); //输出-本地IP
	sub_filter_head = sub_filter_add(sub_filter_head, str1, str2);

	if (strncasecmp(streamin->buff.data, "SIP/2.0 ", strlen("SIP/2.0 ")) != 0) { //sip_request
																				 //process_sip_request
		if (strncasecmp(streamin->buff.data, "REGISTER", strlen("REGISTER"))
				== 0) {
			streamin->stype = SESSION_TYPE_SIP; //第一个数据包必须为 register
			streamout->stype = SESSION_TYPE_SIP;
		} else if (streamin->stype == SESSION_TYPE_NONE) {
			databuff_clear(&streamout->buff); //不能转发哦
//			databuff_append_str(&streamout->buff, "SIP/2.0 403 Forbidden\r\n");
//			databuff_append_str(&streamout->buff,
//					"Warning: 399 devnull \"[Trunk:344946]Not registered\"\r\n");
//			databuff_append_str(&streamout->buff, "Content-Length:0\r\n");
//			databuff_append_str(&streamout->buff, "\r\n");

			databuff_append_str(&streamout->buff,
					"SIP/2.0 401 Unauthorized\r\n");
			databuff_append_str(&streamout->buff, "Content-Length:0\r\n");
			databuff_append_str(&streamout->buff, "\r\n");

			log_d("forward", "not register: %d\n", streamout->buff.pos);
			ret = sock_stream_send(streamin, streamout->buff.data,
					streamout->buff.pos);
			databuff_clear(&streamout->buff);
			databuff_clear(&streamin->buff);
			return ret;
		}
	} else {
		//process_sip_response
	}

	int httplen = string_http_heard_getlength(streamin->buff.data,
			streamin->buff.pos);
	if (httplen > 0) {
		int content_length = string_http_head_get_content_length(
				streamin->buff.data, streamin->buff.pos);

		printf("pack head length:%d, content-length:%d\n", httplen,
				content_length);
		const char *ptr = streamin->buff.data;
		const char *ptr_end = ptr + httplen;
		const char *bk = NULL;
		int linelen = 0;
		char ContentLengthStr[100];

		streamout->buff.pos = 0;
		//
		while (ptr < ptr_end) {
			bk = strchr(ptr, '\n');
			if (bk == NULL)
				break;
			linelen = bk - ptr + 1;

			////Authorization中的内容 不能替换
			if (strncasecmp(ptr, "Authorization:", strlen("Authorization:"))
					== 0) {
				memcpy(streamout->buff.data + streamout->buff.pos, ptr,
						linelen);
			} else if (strncasecmp(ptr, "Content-Length:",
					strlen("Content-Length:")) == 0) {

				sprintf(ContentLengthStr, "Content-Length: %5d\r\n",
						content_length); //65536
				linelen = strlen(ContentLengthStr);
				memcpy(streamout->buff.data + streamout->buff.pos,
						ContentLengthStr, linelen);
			} else {
				string_replace(ptr, linelen,
						streamout->buff.data + streamout->buff.pos, &linelen,
						sub_filter_head);
			}
			streamout->buff.pos += linelen;
			ptr = bk + 1;
		}
		//对剩下的数据替换, ptr为数据内容
		ptr_end = streamin->buff.data + streamin->buff.pos;
		if (ptr < ptr_end) {
			printf(">> Content-Length:%d\n", ptr_end - ptr);

			if (strncasecmp(streamin->buff.data, "INVITE", strlen("INVITE"))
					== 0) {
				//sdp 解析, 媒体服务器地址
				char sdp_serip[30];
				char sdp_media_type[30];
				int sdp_port = 0;

				const char *psdp_ptr = strstr(ptr, "c=IN IP4 ");
				if (psdp_ptr) {
					psdp_ptr += strlen("c=IN IP4 ");
					memset(sdp_serip, 0, sizeof(sdp_serip));
					strncpy(sdp_serip, psdp_ptr,
							string_line_endptr(psdp_ptr) - psdp_ptr);
				}
				psdp_ptr = strstr(ptr, "m=");
				if (psdp_ptr) {
					psdp_ptr += 2;
					sscanf(psdp_ptr, "%s %d", sdp_media_type, &sdp_port);
				}
				//c=IN IP4 192.168.0.16
				//t=0 0
				//m=video 6000 RTP/AVP 96 98 97
				log_d("forward", ">>>> sdp server ip:[%s:%d] %s\n", sdp_serip,
						sdp_port, sdp_media_type); //需要将sdp_ip替换,增加到subfilter
				//iptables -t nat -A PREROUTING -p udp -d 192.168.0.62 --dport 30002 -j DNAT --to 192.168.0.165:3002
				string_replace(ptr, ptr_end - ptr,
						streamout->buff.data + streamout->buff.pos, &linelen,
						_subfiter_sip_INVITE_content);
			} else {
				if (strncasecmp(streamin->buff.data, "SIP/2.0 200 OK",
						strlen("SIP/2.0 200 OK")) == 0) {
					if (strstr(streamin->buff.data, "INVITE")
							&& strstr(streamin->buff.data, "application/sdp")) {
						char *sdp_ip_ptr = strstr(ptr, "IN IP4 ");
						char sdp_ip[20]; //
						if (sdp_ip_ptr) {
							sdp_ip_ptr += strlen("IN IP4 ");
							strcpy_ipv4(sdp_ip, sdp_ip_ptr);

							sprintf(str1, "IN IP4 %s", sdp_ip); //输入-远程IP
							sprintf(str2, "IN IP4 %s", streamout->localip); //输出-本地IP
							sub_filter_head = sub_filter_add(sub_filter_head,
									str1, str2);
						}
					}
				}
				string_replace(ptr, ptr_end - ptr,
						streamout->buff.data + streamout->buff.pos, &linelen,
						sub_filter_head);
			}

			if (linelen != content_length) { //修改 Content-Length:  65535\r\n(63k)
				char *ptr_edit = strstr(streamout->buff.data,
						"Content-Length:");
				bk = strchr(ptr_edit, '\n');
				if (ptr_edit) {
					ptr_edit += strlen("Content-Length: ");
					printf("content-length- edit :%d\n", linelen);
					sprintf(ptr_edit, "%5d", linelen);
					while (ptr_edit < bk) {
						if (*ptr_edit == 0)
							*ptr_edit = '\r';
						ptr_edit++;
					}
				}
			}
			streamout->buff.pos += linelen;
		}
		streamout->buff.data[streamout->buff.pos] = 0;
	}

	sub_filter_free(sub_filter_head);

	ret = sock_stream_send(streamout, streamout->buff.data,
			streamout->buff.pos);

	log_d("forward", "send:%s\n", streamout->description);
//	databuff_out_hex(&streamout->buff, stdout, 16 * 3);
	printf("%s\n", streamout->buff.data);
	databuff_clear(&streamout->buff);
	databuff_clear(&streamin->buff);
	return 0;
}

int forward_session_handle_http(struct forward_session *sess,
		struct sock_stream *streamin, struct sock_stream *streamout) {
	int ret;
	//http头接收
	int httpheadlen = string_http_heard_getlength(streamin->buff.data,
			streamin->buff.pos);

	while (httpheadlen == -1) {
		ret = sock_stream_recv(streamin);
		if (ret == -1)
			return -1;
		httpheadlen = string_http_heard_getlength(streamin->buff.data,
				streamin->buff.pos);
	}

	int http_content_length = string_http_head_get_content_length(
			streamin->buff.data, httpheadlen);

	if (buff_parse_is_onvif_request(streamin->buff.data, streamin->buff.pos)) {
		streamin->stype = SESSION_TYPE_HTTP_REQ_ONVIF;
	}
	if (buff_parse_is_nvr_nuiview_request(streamin->buff.data,
			streamin->buff.pos)) {
		streamin->stype = SESSION_TYPE_HTTP_REQ_NVR_nuiview;
	}

	if (streamin == &sess->sforward_dst) {
		if (sess->sforward_src.stype == SESSION_TYPE_HTTP_REQ_ONVIF) {
			streamin->stype = SESSION_TYPE_HTTP_RES_ONVIF;
		}
		if (sess->sforward_src.stype == SESSION_TYPE_HTTP_REQ_NVR_nuiview) {
			streamin->stype = SESSION_TYPE_HTTP_RES_NVR_nuiview;
		}
	}

	printf("> http_content_length=%d, streamin->stype=%d\n",
			http_content_length, streamin->stype);

	//只替换ONVIF/宇视NVF_cgi报文
	if (SESSION_TYPE_HTTP_REQ_ONVIF == streamin->stype
			|| SESSION_TYPE_HTTP_REQ_NVR_nuiview == streamin->stype
			|| SESSION_TYPE_HTTP_RES_ONVIF == streamin->stype
			|| SESSION_TYPE_HTTP_RES_NVR_nuiview == streamin->stype) {

		//如果没有http_content_length怎么办？边接收边修改
		//只处理：content_length>0 且数据为onvif/nvr/sip
		if (http_content_length > 0) {

			char *http_content_txt = (char*) malloc(http_content_length + 1);
			int http_content_pos = streamin->buff.pos - httpheadlen;

			if (http_content_pos > 0) { //转移http内容
				memcpy(http_content_txt, streamin->buff.data + httpheadlen,
						http_content_pos);
				streamin->buff.pos = httpheadlen; //streamin->buff.data即为http头数据
			}
			streamin->buff.data[streamin->buff.pos] = 0;

			//还需要接收http_content_length-http_content_pos这个数据
			while (http_content_pos < http_content_length) {
				ret = socket_recv(streamin->fd,
						http_content_txt + http_content_pos,
						http_content_length - http_content_pos);
				if (ret < 0) {
					streamin->isclose = 1;
					break;
				}
				if (ret == 0) {
					usleep(1000);
				}
				http_content_pos += ret;
			}

			log_d("forward", "src http head:[%s]\n", streamin->buff.data);
			log_d("forward", "src http content:[%s]\n", http_content_txt);

			if (http_content_pos == http_content_length) { //http内容接收完成

				//处理http数据报文： head(streamin->buff.data) content(http_content_txt)
				if (streamin == &sess->sforward_src) { //修改Content-length:
					int replen = 0;

					string_replace(streamin->buff.data, streamin->buff.pos, //
							streamout->buff.data, &streamout->buff.pos, //
							sess->forward_ser->sub_filter_forwardfromsrc);

					ret = sock_stream_send(streamout, streamout->buff.data,
							streamout->buff.pos);
					ret = sock_stream_send(streamout, http_content_txt,
							http_content_length);
					databuff_clear(&streamout->buff);
				}

				if (streamin == &sess->sforward_dst) {
					int len;
					char *buff = string_replace2(http_content_txt,
							http_content_length, &len, //
							sess->forward_ser->sub_filter_forwardfromdst);
					free(http_content_txt);

					http_content_txt = buff;
					http_content_length = len;
					int replen;

					char *rephstr = string_httphead_replace(streamin->buff.data,
							streamin->buff.pos, &replen, "Content-Length", "%d",
							http_content_length);

					char *rephstr1 = string_replace2(rephstr, replen, &replen,
							sess->forward_ser->sub_filter_forwardfromdst);
					free(rephstr);

					log_d("forward", "send head: %d [%s]\n", replen, rephstr1);
					log_d("forward", "send content: %d [%s]\n",
							http_content_length, http_content_txt);

					ret = sock_stream_send(streamout, rephstr1, replen);
					ret = sock_stream_send(streamout, http_content_txt,
							http_content_length);
					free(rephstr1);
					databuff_clear(&streamin->buff);
					databuff_clear(&streamout->buff);

					streamout->isfirst = 0;
					streamin->isfirst = 0;
				}
			} //内容没有接收完
			free(http_content_txt);
		} else { //不知道Content-length的情况
			int replen;
			char *rep_str;

			log_d("forward", "Content-length not ...\n");

			if (streamin == &sess->sforward_src) {
				rep_str = string_replace2(streamin->buff.data,
						streamin->buff.pos, &replen,
						sess->forward_ser->sub_filter_forwardfromsrc);
			}
			if (streamin == &sess->sforward_dst) {
				rep_str = string_replace2(streamin->buff.data,
						streamin->buff.pos, &replen,
						sess->forward_ser->sub_filter_forwardfromdst);
			}
			log_d("forward", " send to (%s) len:%d %.20s\n",
					streamout->description, replen, rep_str);
			ret = sock_stream_send(streamout, rep_str, replen);
			databuff_clear(&streamin->buff);
			free(rep_str);
		}
	} ////end ONVIF NVR_nuiview
	else {
		//非能私自处理的协议
	}
	return ret;
}

//tcp转发，自适应协议
int forward_session_handle_tcp(struct forward_session *sess,
		struct sock_stream *streamin, struct sock_stream *streamout) {
	int ret;
	int stype = 0;

	log_d("forward", ">>> %s -> %s, len:%d\n", streamin->description,
			streamout->description, streamin->buff.pos);
	databuff_out_hex(&streamin->buff, stdout, 16 * 2);

	if (streamin->isfirst == 0) {
		streamin->isfirst = 1;

		stype = buff_parse_http_req_gettype(streamin->buff.data); //分解协议
		if (stype != SESSION_TYPE_NONE) {
			streamin->stype = SESSION_TYPE_HTTP_REQUEST;
		}

		if (buff_parse_ishttpresponse(streamin->buff.data,
				streamin->buff.pos)) {
			streamin->stype = SESSION_TYPE_HTTP_RESPONSE;
		}

		//属于http协议
		if (SESSION_TYPE_HTTP_RESPONSE == streamin->stype
				|| SESSION_TYPE_HTTP_REQUEST == streamin->stype) {
			forward_session_handle_http(sess, streamin, streamout);
		}
		//sip协议判断
		streamin->buff.data[streamin->buff.pos] = 0;

		if (strstr(streamin->buff.data, "SIP")) {
			//INVITE UPDATE  ACK PRACK BYE REGISTER  | SIP/2.0 200 xxx\r\nCSeq ?? [method]
			ret = forward_session_handle_sip(sess, streamin, streamout);
		}
	} else { //非首次接收的情况

	}
	if (streamin->buff.pos > 0)
		ret = sock_stream_send(streamout, streamin->buff.data,
				streamin->buff.pos);
	databuff_clear(&streamin->buff);
	return ret;
}

static void *_thread_session_tcp(void *args) {
	struct forward_session *sess = (struct forward_session *) args;
	int ret;
	char ptname[16];
	snprintf(ptname, 16, "tcp:%d", sess->sforward_src.fd);
	prctl(PR_SET_NAME, ptname);

	if (sess->forward_ser->proto == IPPROTO_TCP) {
		ret = sock_stream_connect(&sess->sforward_dst,
				sess->forward_ser->f_dst_ip, sess->forward_ser->f_dst_port);

		if (ret == -1) {
			log_d("forward", "err,%s -->[%s:%d]\n",
					sess->sforward_src.description, sess->forward_ser->f_dst_ip,
					sess->forward_ser->f_dst_port);
			close(sess->sforward_src.fd);
			free(sess);
			return NULL;
		}
	} else {
		log_d("forward", "err.....\n");
		free(sess);
		return NULL;
	}

	log_d("forward", "TCP: (%s) <-> (%s)\n", sess->sforward_src.description,
			sess->sforward_dst.description);
//TCP 开始转发咯
	socket_nonblock(sess->sforward_src.fd, 1);
	socket_nonblock(sess->sforward_dst.fd, 1);

	struct pollfd pofds[2];
	int i;
	int loopflag = 1;
	struct sock_stream *streamin, *streamout;

	while (1) {
		memset(pofds, 0, sizeof(pofds));

		pofds[0].fd = sess->sforward_src.fd;
		pofds[1].fd = sess->sforward_dst.fd;
		pofds[0].events = POLLIN;
		pofds[1].events = POLLIN;

		ret = poll(pofds, 2, 200);
		if (ret <= 0) {
			if (errno == EAGAIN || errno == EINTR || ret == 0)
				continue;
			log_d("forward", "poll err:%d:%s\n", errno, strerror(errno));
			break;
		}
////
		for (i = 0; i < 2; i++) {
			if (pofds[i].revents & (POLLERR | POLLNVAL | POLLHUP)) {
				log_d("forward", "POLLERR\n");
				loopflag = 0;
				break;
			}
		}
		if (!loopflag)
			break;

		for (i = 0; i < 2; i++) {
			if (POLLIN == (pofds[i].revents & POLLIN)) {
				if (i == 0) {
					streamin = &sess->sforward_src;
					streamout = &sess->sforward_dst;
				}
				if (i == 1) {
					streamin = &sess->sforward_dst;
					streamout = &sess->sforward_src;
				}
				ret = sock_stream_recv(streamin);
				if (ret == -1) {
					log_d("forward", "need close:%s\n", streamin->description);
					loopflag = 0;
					break;
				}
				if (sess->forward_ser->forward_handle) {
					ret = sess->forward_ser->forward_handle(sess, streamin,
							streamout);
				} else {
					ret = forward_session_handle_tcp(sess, streamin, streamout); //默认TCP转发
				}
			}
		}

//		if (POLLIN == (pofds[0].revents & POLLIN)) {
//			ret = forward_session_handle_tcp(sess, &sess->sforward_src,
//					&sess->sforward_dst);
//			if (ret < 0) {
//				loopflag = 0; //close...
//				break;
//			}
//		}
//		if (POLLIN == (pofds[1].revents & POLLIN)) {
//			ret = forward_session_handle_tcp(sess, &sess->sforward_dst,
//					&sess->sforward_src);
//			if (ret < 0) {
//				loopflag = 0; //close...
//				break;
//			}
//		}
	}
	close(sess->sforward_dst.fd);
	close(sess->sforward_src.fd);

	log_d("forward", "thread exit, close: (%s)(%s)\n",
			sess->sforward_src.description, sess->sforward_src.description);
	free(sess);
	return NULL;
}

//EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
//EPOLLOUT：表示对应的文件描述符可以写；
//EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
//EPOLLERR：表示对应的文件描述符发生错误；
//EPOLLHUP：表示对应的文件描述符被挂断；
//EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
//EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
int epoll_set(int epfd, int fd, int opt, int events, void *ptr) {
	struct epoll_event event;
	event.events = events;
	event.data.ptr = ptr;
	return epoll_ctl(epfd, opt, fd, &event);
}

/**************
 每个udp包的最大大小是多少?
 65507 约等于 64K

 为什么最大是65507?
 因为udp包头有2个byte用于记录包体长度. 2个byte可表示最大值为: 2^16-1=64K-1=65535
 udp包头占8字节, ip包头占20字节, 65535-28 = 65507
 ***************/
void forward_server_dispatcher(struct list_head *list_fserver) {
	int i = 0;
	struct socket_server *sockser;
	struct list_head list_sock_fserver;
	struct list_head *pos, *n;
	struct forward_server *fserver;

	INIT_LIST_HEAD(&list_sock_fserver);

	uint8_t buff[1024 * 60]; //60k

	list_for_each(pos, list_fserver)
	{
		fserver = list_entry(pos, struct forward_server, list);

		sockser = socket_server_build(fserver);
		if (sockser != NULL) {
			list_add(&sockser->list, &list_sock_fserver);
			log_d("forward", "create server(%s):%d(%s,%s:%d -> %s:%d)\n",
					sockser->forward_ser->server_name, sockser->fd,
					sockser->forward_ser->proto==IPPROTO_TCP?"TCP":"UDP",
					sockser->forward_ser->f_server_ip,
					sockser->forward_ser->f_server_port,
					sockser->forward_ser->f_dst_ip,
					sockser->forward_ser->f_dst_port);
		} else {
			log_d("forward", "server build err\n");
		}
	}

	int epfd = epoll_create1(0);
	int eplen = 0;

	struct epoll_event events[100];

	list_for_each(pos, &list_sock_fserver)
	{
		sockser = list_entry(pos, struct socket_server, list);
		epoll_set(epfd, sockser->fd, EPOLL_CTL_ADD, EPOLLIN, sockser);
	}

	int ret;
	int event_is_server = 0;

	struct list_head list_fsession_udp;
	INIT_LIST_HEAD(&list_fsession_udp);

	while (1) {
		eplen = epoll_wait(epfd, events, 100, 100);
		if (eplen == 0) {
			struct forward_session *sess = NULL;
			list_for_each_safe(pos,n, &list_fsession_udp)
			{
				sess = list_entry(pos, struct forward_session, list);
				if (timer_expired(&sess->sforward_dst.timeoutms)) {
					log_d("forward", "已超时(%s) ... \n",
							sess->sforward_dst.description);
					epoll_set(epfd, sess->sforward_dst.fd, EPOLL_CTL_DEL,
					EPOLLIN, NULL);
					list_del(&sess->list);

					close(sess->sforward_dst.fd);
					forward_session_free(sess);
				}
			}
		}
		if (eplen <= 0) {
			if (errno == EAGAIN || errno == EINTR || eplen == 0)
				continue;
			perror("epoll wait!\n");
			break;
		}

		for (i = 0; i < eplen; i++) {
			if (events[i].events & EPOLLIN == EPOLLIN) {
				event_is_server = 0;

				list_for_each(pos, &list_sock_fserver)
				{
					sockser = list_entry(pos, struct socket_server, list);
					if (events[i].data.ptr == sockser) {
						event_is_server = 1;
						break;
					}
				}

				if (event_is_server && sockser) {

					if (sockser->forward_ser->proto == IPPROTO_TCP) {
						struct forward_session *sess = forward_session_new();
						sess->forward_ser = sockser->forward_ser;
						ret = sock_stream_accept(&sess->sforward_src,
								sockser->fd);
						if (ret == 0) {
							pthread_t pt;
							pthread_attr_t attr;
							pthread_attr_init(&attr);
							pthread_attr_setdetachstate(&attr,
							PTHREAD_CREATE_DETACHED);
							pthread_create(&pt, &attr, _thread_session_tcp,
									sess);
							pthread_attr_destroy(&attr);
						} else {
							free(sess);
						}
					}

					if (sockser->forward_ser->proto == IPPROTO_UDP) {
						//需要查找转发会话
						struct sockaddr_in addrin;
						int addrlen = sizeof(addrin);
						char fsrc_ip[30];
						int fsrc_port;
						int bufflen = 0;

						memset(buff, 0, sizeof(buff));
						ret = recvfrom(sockser->fd, buff, sizeof(buff), 0,
								(struct sockaddr *) &addrin, &addrlen);
						//依据会话寻找UDP转发出 会话线程
						sockaddr_to_ipinfo(&addrin, fsrc_ip, &fsrc_port);

						if (ret > 0) {
							bufflen = ret;
							struct forward_session *sess = NULL;

							list_for_each(pos, &list_fsession_udp)
							{
								sess = list_entry(pos, struct forward_session,
										list);
								if (strcmp(sess->sforward_src.remoteip, fsrc_ip)
										== 0
										&& sess->sforward_src.remoteport
												== fsrc_port) {
									break;
								}
								sess = NULL;
							}

							if (sess == NULL) {
								sess = forward_session_new();
								sess->forward_ser = sockser->forward_ser;
								sess->sforward_src.fd = sockser->fd;
								sess->sforward_src.proto = IPPROTO_UDP;

								sock_stream_set_localaddr(&sess->sforward_src,
										sockser->forward_ser->f_server_ip,
										sockser->forward_ser->f_server_port);
								sock_stream_set_remoteaddr(&sess->sforward_src,
										fsrc_ip, fsrc_port);

								sock_stream_descript(&sess->sforward_src, 0);

								ret = sock_stream_bind_udp_toremote(
										&sess->sforward_dst,
										sockser->forward_ser->f_out_ip, 0,
										sockser->forward_ser->f_dst_ip,
										sockser->forward_ser->f_dst_port);

								if (ret == -1) {
									free(sess);
									sess = NULL;
								} else {
									log_d("forward",
											"new session [%s <-> %s]\n",
											sess->sforward_src.description,
											sess->sforward_dst.description);

									timer_set(&sess->sforward_dst.timeoutms,
											1000 * 3600); // 一小时
									epoll_set(epfd, sess->sforward_dst.fd,
									EPOLL_CTL_ADD, EPOLLIN, sess);
									list_add(&sess->list, &list_fsession_udp);
								}
							}

							memcpy(sess->sforward_src.buff.data, buff, bufflen);
							sess->sforward_src.buff.pos = bufflen;
							sess->sforward_src.buff.data[bufflen] = 0;

							if (sess->forward_ser->forward_handle) {
								sess->forward_ser->forward_handle(sess,
										&sess->sforward_src,
										&sess->sforward_dst);
							} else {
								forward_session_handle_udp(sess,
										&sess->sforward_src,
										&sess->sforward_dst);
							}
						}
					}
					continue;
				} // end event_is_server

				{
					struct forward_session *sess = NULL;
					list_for_each(pos, &list_fsession_udp)
					{
						sess = list_entry(pos, struct forward_session, list);
						if (events[i].data.ptr == sess) {
							memset(&sess->sforward_dst.buff.data, 0,
									sess->sforward_dst.buff.size);

							//ret = sock_stream_recvfrom(&sess->sforward_dst);
							int bufflen = 0;
							struct sockaddr_in addrin;
							socklen_t addrlen;
							char ipstr[30];
							int port;

							memset(buff, 0, sizeof(buff));
							bufflen = recvfrom(sess->sforward_dst.fd, buff,
									sizeof(buff), 0,
									(struct sockaddr *) &addrin, &addrlen);
							//依据会话寻找UDP转发出 会话线程
							sockaddr_to_ipinfo(&addrin, ipstr, &port);

							if (port == sess->sforward_dst.remoteport
									&& strcmp(ipstr,
											sess->sforward_dst.remoteip) == 0) {
								memcpy(sess->sforward_dst.buff.data, buff,
										bufflen);
								sess->sforward_dst.buff.pos = bufflen;
								sess->sforward_dst.buff.data[bufflen] = 0;

								timer_restart(&sess->sforward_dst.timeoutms);

								if (sess->forward_ser->forward_handle) {
									sess->forward_ser->forward_handle(sess,
											&sess->sforward_dst,
											&sess->sforward_src);
								} else {
									forward_session_handle_udp(sess,
											&sess->sforward_dst,
											&sess->sforward_src);
								}
							} else {
								log_d("forward", "未知转发数据:%s:%d\n", ipstr, port);
							}

						}
					}
				}
			}
		}
	}
}

#if 0
#include "uci_mapping.h"

static char ip_wan[50]; //GE
static char ip_lan[50];//ETH

static void _bk_GE2ETH(struct CfgGE2ETH *cfg, void *usr)
{
	struct forward_server *fserver;
	struct list_head *list_forware = (struct list_head*) usr;
	char str1[50];
	char str2[50];

	if (strcasecmp(cfg->type, "command") == 0 || strcasecmp(cfg->type, "all") == 0)
	{
		fserver = (struct forward_server *) calloc(sizeof(struct forward_server), 1);
		fserver->proto = IPPROTO_TCP;
		snprintf(fserver->server_name, 50, "SIP:%s", cfg->cfgname);

		strcpy(fserver->f_server_ip, cfg->dstIP);
		fserver->f_server_port = cfg->gb28181Port;

		strcpy(fserver->f_dst_ip, cfg->srcIP);
		fserver->f_dst_port = cfg->gb28181Port;

		strcpy(fserver->f_out_ip, ip_wan);

		fserver->forward_handle = forward_session_handle_sip;
		list_add(&fserver->list, list_forware);

		fserver = (struct forward_server *) calloc(sizeof(struct forward_server), 1);
		fserver->proto = IPPROTO_UDP;
		snprintf(fserver->server_name, 50, "SIP:%s", cfg->cfgname);

		strcpy(fserver->f_server_ip, cfg->dstIP);
		fserver->f_server_port = cfg->gb28181Port;

		strcpy(fserver->f_dst_ip, cfg->srcIP);
		fserver->f_dst_port = cfg->gb28181Port;

		strcpy(fserver->f_out_ip, ip_wan);

		fserver->forward_handle = forward_session_handle_sip;
		list_add(&fserver->list, list_forware);
	}

	if (strcasecmp(cfg->type, "media") == 0 || strcasecmp(cfg->type, "all") == 0)
	{
		sprintf(str1, "c=IN IP4 %s", cfg->srcIP);
		sprintf(str2, "c=IN IP4 %s", cfg->dstIP);
		sip_INVITE_content_subfilter(str1, str2);
	}
}

int main(int argc, char **argv)
{
	uci_get("network.wan.ipaddr", ip_wan);
	uci_get("network.lan.ipaddr", ip_lan);

	log_d("forward", "wan.ip:%s\n", ip_wan);
	log_d("forward", "lan.ip:%s\n", ip_lan);
	LIST_HEAD(list_forware);

	uci_mapping_foreach_GE2ETH(_bk_GE2ETH, &list_forware);

	log_d("forward", "start dispatcher\n");
	forward_server_dispatcher(&list_forware);
}

#endif

#if 1

//本地服务转发:本地监听socket,链接到目标IP,进行转发
//远程转发:链接两个socket对内容转发
struct remote_forward_session {
	struct list_head list;
	struct sock_stream stream1;
	struct sock_stream stream2;
	int flag_set; // 1:已经添加到epollfd
	int flag_close;
};

static struct list_head list_forward;
static pthread_mutex_t list_forward_mutex = PTHREAD_MUTEX_INITIALIZER;
static int dummy_pipe_fds[2]; //事件通知

#ifdef CONFIG_USE_PTHREAD_FORWARD

void *_thread_forward2(void *arg) {
	pthread_detach(pthread_self());
	struct remote_forward_session *session =
	(struct remote_forward_session*) arg;

	//非阻塞
	log_d("forward", "转发中: [%d:%d]\n", session->stream1.fd,
			session->stream2.fd);

	socket_nonblock(session->stream1.fd, 1);
	socket_nonblock(session->stream2.fd, 1);

	struct pollfd pofds[2];
	int loopflag = 1;
	int i = 0;
	int rlen = 0;
	int fd_to;
	int ret;

	while (loopflag) {
		memset(pofds, 0, sizeof(pofds));
		pofds[0].fd = session->stream1.fd;
		pofds[1].fd = session->stream2.fd;
		pofds[0].events = POLLIN;
		pofds[1].events = POLLIN;

		ret = poll(pofds, 2, 200);
		if (ret <= 0) {
			if (errno == EAGAIN || errno == EINTR || ret == 0)
			continue;
			log_d("forward", "poll err:%d:%s\n", errno, strerror(errno));
			break;
		}

		for (i = 0; i < 2; i++) {
			if (pofds[i].revents & (POLLERR | POLLNVAL | POLLHUP)) {
				log_d("forward", "POLLERR\n");
				loopflag = 0;
				break;
			}
		}
		if (!loopflag)
		break;

		for (i = 0; i < 2; i++) {
			if (POLLIN == (pofds[i].revents & POLLIN)) {
				struct sock_stream *streamin, *streamout;
				if (i == 0) {
					streamin = &session->stream1;
					streamout = &session->stream2;
				}
				if (i == 1) {
					streamin = &session->stream2;
					streamout = &session->stream1;
				}

				ret = sock_stream_recv(streamin);
				if (ret == -1) {
					log_d("forward", "need close:%s\n", streamin->description);
					loopflag = 0;
					break;
				}

				log_d("forward", "[%s]->[%s]\n", streamin->description,
						streamout->description);
				databuff_out_hex(&streamin->buff, stdout, 100);
				ret = sock_stream_send(streamout, streamin->buff.data,
						streamin->buff.pos);
				streamin->buff.pos = 0;
			}
		}
	}
	close(session->stream1.fd);
	close(session->stream2.fd);
	log_d("forward", "need close:%s\n", session->stream1.description);
	log_d("forward", "need close:%s\n", session->stream2.description);
	free(session);
	return NULL;
}
#endif

void remote_forward_register(int fd1, int fd2) {
	struct remote_forward_session *session =
			(struct remote_forward_session*) calloc(
					sizeof(struct remote_forward_session), 1);
	sock_stream_build_tcp(&session->stream1, fd1);
	sock_stream_build_tcp(&session->stream2, fd2);
	session->stream1.puser = session;
	session->stream2.puser = session;

	session->stream1.buff.size = DATABUFF_SIZE - 10;
	session->stream2.buff.size = DATABUFF_SIZE - 10;
	session->flag_set = 0;
	session->flag_close = 0;

#ifdef CONFIG_USE_PTHREAD_FORWARD
	pthread_t pt;
	pthread_create(&pt, NULL, _thread_forward2, session);
#else
	pthread_mutex_lock(&list_forward_mutex);
	list_add_tail(&session->list, &list_forward);
	pthread_mutex_unlock(&list_forward_mutex);
	char buf = 0;

	log_d("forward", "注册开始\n");
	write(dummy_pipe_fds[1], &buf, 1);
#endif
}

void *thread_forward(void *args) {
	int epfd = epoll_create1(0);
	int epwait_ret = 0;
	struct epoll_event events[1024];
	struct epoll_event *epevent = NULL;
	int i;
	int ret;

	INIT_LIST_HEAD(&list_forward);

	prctl(PR_SET_NAME, "forward#");

	if (pipe(dummy_pipe_fds)) { //fd[0] → r； fd[1] → w
		log_e("forward", "Unable to create pipe\n");
	}

	log_d("forward", "远程转发线程开启:epfd:%d, pipe:%d,%d\n", epfd, dummy_pipe_fds[0],
			dummy_pipe_fds[1]);

	epoll_set(epfd, dummy_pipe_fds[0], EPOLL_CTL_ADD, EPOLLIN, &dummy_pipe_fds);

	while (1) {
		{
			struct remote_forward_session *session;
			struct list_head *p, *n;

			pthread_mutex_lock(&list_forward_mutex);
			list_for_each_safe(p,n, &list_forward)
			{
				session = list_entry(p, struct remote_forward_session, list);
				if (session->flag_close == 1) {
					epoll_set(epfd, session->stream1.fd, EPOLL_CTL_DEL, EPOLLIN,
					NULL);
					epoll_set(epfd, session->stream2.fd, EPOLL_CTL_DEL, EPOLLIN,
					NULL);
					close(session->stream1.fd);
					close(session->stream2.fd);
					log_d("forward", "need close:%s\n",
							session->stream1.description);
					log_d("forward", "need close:%s\n",
							session->stream2.description);
					list_del(&session->list);
					session->flag_set = -1;
					free(session);
				}
			}
			pthread_mutex_unlock(&list_forward_mutex);
		}
		epwait_ret = epoll_wait(epfd, events, 1024, 300);
		if (epwait_ret == 0)
			continue;
		log_d("epoll", "wait ret=%d\n", epwait_ret);
		for (i = 0; i < epwait_ret; i++) {
			epevent = &events[i]; //如果监听的是读事件，并返回的是读事件,需要读取消息,然后将其转发出去

			//通知事件消息
			if (epevent->data.ptr == &dummy_pipe_fds) {
				if ((epevent->events & EPOLLIN) == EPOLLIN) {
					char buf;
					log_d("forward", "需要刷新列表哦\n");
					read(dummy_pipe_fds[0], &buf, 1); //接收到事件
					{
						struct remote_forward_session *session;
						list_for_each_entry(session, &list_forward, list)
						{
							if (session->flag_set == 0) {
								log_d("forward", "远程转发线程注册 %d %d\n",
										session->stream1.fd,
										session->stream2.fd);
								epoll_set(epfd, session->stream1.fd,
								EPOLL_CTL_ADD, EPOLLIN, &session->stream1); //添加到epoll

								epoll_set(epfd, session->stream2.fd,
								EPOLL_CTL_ADD, EPOLLIN, &session->stream2); //添加到epoll
								session->flag_set = 1;  //添加标志
							}
						}
					}
				}
				continue;
			}

			////////////////////////////////////////////////////////////////
			if ((epevent->events & EPOLLIN) == EPOLLIN) {
				struct remote_forward_session *session;
				struct sock_stream *streamin = epevent->data.ptr;
				struct sock_stream *streamout;

				session = streamin->puser;

				if (streamin == &session->stream1)
					streamout = &session->stream2;
				else
					streamout = &session->stream1;

				streamin->buff.pos = 0; //接收位置,也表示当前buff可用数据长度设为0
				streamin->buff.pread = 0; //读取未知
				ret = sock_stream_recv(streamin);
				if (ret == 0)  //返回0 非阻塞退出, 表示需要再次接收
					continue;

				if (ret < 0) {
					log_d("epoll", "recv err(ret=%d):%s, errno=%d:%s\n", ret,
							streamin->description, errno, strerror(errno));
				}

				if (ret > 0) {
					log_d("forward", "接收数据[%s] %d\n",
												streamin->description,
												streamin->buff.pos);
					databuff_out_hex(&streamin->buff, stdout, 100); //streamin->buff.pos);
//						int err;
//						socklen_t len=sizeof(int);
//						if(!getsockopt(streamout->fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len)){
//						}
#define CONFIG_USE_SYNC 0
#if CONFIG_USE_SYNC
					log_d("forward", "转发数据[%s]->[%s] %d\n",
							streamin->description, streamout->description,
							streamin->buff.pos);
					databuff_out_hex(&streamin->buff, stdout, 100); //streamin->buff.pos);

					//将数据转发到另外的fd
					ret = sock_stream_send(streamout, streamin->buff.data,
							streamin->buff.pos);
					if (ret < 0) {
						log_d("epoll", "send err(%d):%s\n", ret,
								streamout->description);
					}
					streamin->buff.pos = 0;
#else
					//让streamin 不再需要读, streamout 需要写
					epoll_set(epfd, streamin->fd, EPOLL_CTL_MOD, EPOLLERR,
							streamin); //修改
					epoll_set(epfd, streamout->fd, EPOLL_CTL_MOD, EPOLLOUT,
							streamout); //修改
#endif

				}

				if (ret == -1)
					session->flag_close = 1; //需要close
				continue;
			} ///EPOLLIN end

			//可写状态
			if ((epevent->events & EPOLLOUT) == EPOLLOUT) {
				struct remote_forward_session *session;
				struct sock_stream *stream_write = epevent->data.ptr;
				struct sock_stream *stream_src = NULL;

				session = stream_write->puser;

				if (stream_write == &session->stream1)
					stream_src = &session->stream2;
				else
					stream_src = &session->stream1;

				//需要将 stream_src->buff 写到 stream_write, 如果写入完成,则修改 src为读模式
				log_d("forward", "转发数据[%s]->[%s] %d\n", stream_src->description,
						stream_write->description, stream_src->buff.pos);
				databuff_out_hex(&stream_src->buff, stdout, 100); //streamin->buff.pos);

				int sret = 0;

				sret = send(stream_write->fd,
						stream_src->buff.data + stream_src->buff.pread,
						stream_src->buff.pos - stream_src->buff.pread, 0);
				if (sret > 0)
					stream_src->buff.pread += sret;

				if (sret <= 0) { //中断，需要重新发送
					if (sret < 0
							&& (errno == EAGAIN || errno == EWOULDBLOCK
									|| errno == EINTR)) {
						//阻塞中,需要下次发送
					} else {
						session->flag_close = 1; //发送错误 需要关闭
					}
				}

				if (stream_src->buff.pread == stream_src->buff.pos) {
					//修改为读模式
					stream_src->buff.pread = 0;
					stream_src->buff.pos = 0;

					epoll_set(epfd, stream_write->fd, EPOLL_CTL_MOD, EPOLLIN,
							stream_write);
					epoll_set(epfd, stream_src->fd, EPOLL_CTL_MOD, EPOLLIN,
							stream_src);
				}
			}

			if (epevent->events & (EPOLLERR | EPOLLHUP)) {
				struct remote_forward_session *session;
				struct sock_stream *streamin = epevent->data.ptr;
				session = streamin->puser;
				session->flag_close = 1; //需要close
				log_d("epoll", "event[%d] is EPOLLERR\n", epevent->events);
			}
		}
	}
	return NULL;
}

void socket_forward_server_start() {
	pthread_t pt;
	pthread_create(&pt, NULL, thread_forward, NULL);
}

int socket_forward_start(const char *forwardhead, const char *ipserver,
		int portserver, const char *ipdst, int portdst) {
	int fd_src;
	int fd_dst;

	log_d("forward", "开始转发:[%s]\n", forwardhead);
	fd_src = socket_connect(ipserver, portserver);

	if (fd_src == -1) {
		log_d("forward", "链接错误:[%s:%d]\n", ipserver, portserver);
		return -1;
	}

	socket_send(fd_src, forwardhead, strlen(forwardhead));
	socket_send(fd_src, "\r\n", 2);

	fd_dst = socket_connect(ipdst, portdst);
	if (fd_dst == -1) {
		close(fd_src);
		log_d("forward", "链接错误:[%s:%d]\n", ipdst, portdst);
		return -1;
	}
	log_d("forward", "异步传输开始:[%d:%d]\n", fd_src, fd_dst);
	remote_forward_register(fd_src, fd_dst);
	return 0;
}

#endif
