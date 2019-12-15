/****
 *
 *
 ****/
#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <linux/tcp.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <signal.h>

/* 功  能：将str字符串中的oldstr字符串替换为newstr字符串
 * 参  数：str：操作目标 oldstr：被替换者 newstr：替换者
 * 返回值：返回替换之后的字符串
 * 版  本： V0.2
 */
char *strrpc(char *str, char *oldstr, char *newstr)
{
	char bstr[strlen(str)]; //转换缓冲区
	memset(bstr, 0, sizeof(bstr));

	for (int i = 0; i < strlen(str); i++)
	{
		if (!strncmp(str + i, oldstr, strlen(oldstr)))
		{ //查找目标字符串
			strcat(bstr, newstr);
			i += strlen(oldstr) - 1;
		}
		else
		{
			strncat(bstr, str + i, 1); //保存一字节进缓冲区
		}
	}

	strcpy(str, bstr);
	return str;
}

char *str_replace(const char *src, const char *strold, const char *strnew, char *strto, int tolen, int *pnewlen)
{
	const char *src_end = src + strlen(src);
	const char *psrc = src;
	char *pto = strto;
	const char *pto_end = strto + tolen;

	int oldlen = strlen(strold);
	int newlen = strlen(strnew);

	while (psrc < src_end && pto < pto_end)
	{
		if (strncmp(psrc, strold, oldlen) == 0)
		{
			if (pto + newlen >= pto_end)
			{
				printf(">>>>>>>err strcpy\n");
				break;
			}
			strcpy(pto, strnew); //pto = mempcpy(pto, strnew, newlen);
			pto += newlen;
			psrc += oldlen;
			continue;
		}
		*pto++ = *psrc++;
	}

	assert(pto <= pto_end);

	if (pto == pto_end)
		pto--;
	*pto = 0;

	if (pnewlen)
		*pnewlen = pto - strto;

	return strto;
}
/**
 * 字符串替换方法1,需要释放返回值
 */
char *str_replace_new(const char *str, const char *src, const char *dst)
{
	const char *pos = str;
	const char *src_end = str + strlen(str);

	int len_old = strlen(src);
	int len_dst = strlen(dst);
	size_t result_len = 1;

	int count = 0;

	while (pos < src_end)
	{
		if (strncmp(pos, src, len_old) == 0)
		{
			pos += len_old;
			result_len += len_dst;
			continue;
		}
		pos++;
		result_len++;
	}

	char *result = (char *)malloc(result_len); //计算个数
	char *pdst = result;
	if (result == NULL)
		return result;

	memset(result, 0, result_len);

	pos = str;

	//printf("result_len=%d\n", result_len);

	while (pos < src_end)
	{
		if (strncmp(pos, src, len_old) == 0)
		{
			strcpy(pdst, dst);
			pdst += len_dst;
			pos += len_old;
			continue;
		}
		*pdst++ = *pos++;
	}
	*pdst = 0;

	return result;
}

int logprintf(const char *file, const char *function, int line, const char *format, ...)
{
	char tmstr[20];
	struct tm tm;
	struct timeval tp;
	gettimeofday(&tp, NULL);
	localtime_r(&tp.tv_sec, &tm);
	strftime(tmstr, sizeof(tmstr), "%F %T", &tm);
	printf("%s,%s,%s,%d:", tmstr, file, function, line);
	va_list v;
	va_start(v, format);
	vprintf(format, v);
	va_end(v);
	return 0;
}

#define log_d(format, ...) logprintf(__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

//非阻塞模式
static inline int socket_nonblock(int fd, int flag)
{
	int opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1)
	{
		return -1;
	}
	if (flag)
		opt |= O_NONBLOCK;
	else
		opt &= ~O_NONBLOCK;

	if (fcntl(fd, F_SETFL, opt) == -1)
	{
		return -1;
	}
	return 0;
}

int socket_connect(const char *ip, int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int ret;
	if (fd == -1)
		return fd;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);

	ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1)
		goto _reterr;

	return fd;
_reterr:
	if (fd != -1)
		close(fd);
	return -1;
}

int socket_bind_tcp(const char *ip, int port, int reuseport, int reuseaddr)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int ret;
	if (fd == -1)
		return fd;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	inet_aton(ip, &addr.sin_addr);
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));
	if (ret == -1)
		goto _reterr;
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1)
		goto _reterr;

	ret = listen(fd, 10);
	if (ret == -1)
		goto _reterr;

	return fd;
_reterr:
	if (fd != -1)
		close(fd);
	return -1;
}

struct addrinfo
{
	char ipstr[20];
	int port;
	int fd;
};

//转发服务
struct repeater_server
{
	struct addrinfo serveraddr; //转发器服务地址
	struct addrinfo toaddr;		//被转发到地址
	int fd;
};

enum protocoltype
{
	PROTO_TYPE_NONE,
	PROTO_TYPE_HTTP_REQUEST,
	PROTO_TYPE_HTTP_RESPONSE
	//PROTO_TYPE_HTTP_REQ_ONVIF
};

struct proto_http_info
{
	int isonvif_head;
	int iskeep_alive;
	int contentLength;

	//int contentEncoding;
	//Content-Encoding: gzip

	//int contentType;
	//Content-Type: video/mp4
	//Content-Type: text/html;charset=utf-8
	//Transfer-Encoding: chunked
};

struct sockstream
{
	int fd;
	int type; //in=0,  out=1

	struct sockaddr_in addrlocal;
	struct sockaddr_in addrremote;
	char localip[20];
	int local_port;

	char remoteip[20];
	int remote_port;

	enum protocoltype protocol; //0=unknow, 1=http
	struct proto_http_info info_http_head;

	int tm_lastr; //最后接收时间
	int tm_lastw; //最后发送时间
	int isfirst;  //首次接收情况
};

struct repeater_stream
{
	struct repeater_stream *next;
	struct repeater_stream *prev;
	struct repeater_server repserver;

	struct sockstream instream;  //接受到的数据中有自己的IP,需要修改为repserip,然后发送给dscaddr
	struct sockstream outstream; //接受到的数据中有自己的IP,需要修改为repserip, 然后发送给inaddr
};

void repeater_stream_reg(struct repeater_stream *stream)
{
}

void repeater_stream_unreg(struct repeater_stream *stream)
{
}

int sockstream_debug_print(struct sockstream *stream)
{
	if (stream->type == 0)
		printf("in %d (%s:%d-%s:%d) \n", stream->fd, stream->remoteip, stream->remote_port, stream->localip, stream->local_port);
	else
		printf("out %d (%s:%d-%s:%d) \n", stream->fd, stream->localip, stream->local_port, stream->remoteip, stream->remote_port);
	return 0;
}

int repeater_stream_debug_print(struct repeater_stream *repstream)
{
	printf("-------------------------------\n");
	printf("[%s:%d] in:%d (%s:%d->%s:%d) ", repstream->repserver.serveraddr.ipstr,
		   repstream->repserver.serveraddr.port, repstream->instream.fd, repstream->instream.remoteip,
		   repstream->instream.remote_port, repstream->instream.localip,
		   repstream->instream.local_port);

	printf("[%s:%d] out:%d (%s:%d->%s:%d)\n", repstream->repserver.toaddr.ipstr,
		   repstream->repserver.toaddr.port, repstream->outstream.fd, repstream->outstream.localip,
		   repstream->outstream.local_port, repstream->outstream.remoteip,
		   repstream->outstream.remote_port);
	printf("-------------------------------\n");
	return 0;
}

int sockstream_send(struct sockstream *stream, void *data, size_t len)
{
	int ret;
	ret = send(stream->fd, data, len, 0);
	if (ret > 0 && len > 0)
	{
		stream->tm_lastw = time(NULL);
	}
	return ret;
}

int sockstream_recv(struct sockstream *stream, void *data, size_t len)
{
	int ret = recv(stream->fd, data, len, 0);
	if (ret > 0 && len > 0)
	{
		stream->tm_lastr = time(NULL);
	}
	return ret;
}

void repeater_server_init(struct repeater_server *item, const char *serverip, int serverport,
						  const char *dscip, int dscport)
{
	strcpy(item->serveraddr.ipstr, serverip);
	item->serveraddr.port = serverport;
	strcpy(item->toaddr.ipstr, dscip);
	item->toaddr.port = dscport;
}

int ishttprequest(char *buff, int len)
{
	//1、GET;2、POST;3、PUT;4、DELETE;5、HEAD;6、TRACE;7、OPTIONS;
	static char *str[] = {"POST", "GET", "PUT", "DELETE", "HEAD", "TRACE", "OPTIONS"};
	for (size_t i = 0; i < 7; i++)
	{
		if (strncasecmp(str[i], buff, strlen(str[i])) == 0)
		{
			if (' ' == buff[strlen(str[i])])
				return 1;
		}
	}
	return 0;
}

int ishttpresponse(char *buff, int len)
{
	if (strncasecmp(buff, "HTTP", 4) == 0)
	{
		return 1;
	}
	return 0;
}

int http_heard_end(const char *buf, int len)
{
	const char *end = buf + len;
	const char *ptr = buf;

	while (ptr < end)
	{
		if (ptr + 1 < end && *ptr == '\n' && *(ptr + 1) == '\n')
		{
			return ptr - buf + 2;
		}
		if (ptr + 2 < end && *ptr == '\n' && *(ptr + 1) == '\r' && *(ptr + 2) == '\n')
		{
			return ptr - buf + 3;
		}
		if (ptr + 2 < end && *ptr == '\r' && *(ptr + 1) == '\n' && *(ptr + 2) == '\n')
		{
			return ptr - buf + 3;
		}
		if (ptr + 3 < end && *ptr == '\r' && *(ptr + 1) == '\n' && *(ptr + 2) == '\r' && *(ptr + 3) == '\n')
		{
			return ptr - buf + 4;
		}
		ptr++;
	}

	return -1;
}

int socket_recv(int fd, void *data, int size)
{
	int ret;
	int rlen = 0;

	while (size)
	{
		ret = recv(fd, (uint8_t *)data + rlen, size, 0);
		if (ret == 0)
			return -1;
		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		}
		size -= ret;
		rlen += ret;
	}
	return rlen;
}

//如果一直阻塞呢?怎么办?
int socket_send(int fd, void *data, int size)
{
	int slen = 0;
	int ret;
	while (slen < size)
	{
		ret = send(fd, (uint8_t *)data + slen, size - slen, 0);
		if (ret == 0)
		{
			slen = -1;
			break;
		}
		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
			{
				//printf("eagin :%d\n", fd);
				continue;
			}
			slen = -1;
			break;
		}
		slen += ret;
	}
	return slen;
}

int socket_recv_line(int fd, char *linestr, int maxsize)
{
	int ret;
	int rlen = 0;
	char v;
	char *ptr = linestr;

	while (rlen < maxsize)
	{
		ret = recv(fd, &v, 1, 0);
		if (ret == 0)
			return -1;

		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		}
		//log_d(" %c ", v);
		*ptr++ = v;
		rlen++;

		if (v == '\n')
		{
			*ptr++ = 0;
			break;
		}
	}

	return rlen;
}

int poll_isout(int fd)
{
	struct pollfd pfd;
	int ret;
	pfd.fd = fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;
	ret = poll(&pfd, 1, 0);
	return pfd.revents & POLLOUT;
}

struct buffdata
{
	char data[1024 * 20]; //20K
	int index;
	int size;
};

int _loop_http_head_request_in_out(struct sockstream *in, struct sockstream *out, struct buffdata *buffcache)
{
	int httpheadlen;
	char cacheline[1024];
	int linelen;
	int rlen = 0;

	httpheadlen = http_heard_end(buffcache->data, buffcache->index);
	while (httpheadlen == -1)
	{
		rlen = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index); //预留100字节做备份

		if (rlen <= 0)
		{
			if (rlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
			{
				usleep(1000);
				continue;
			}

			log_d("recv err, rlen=%d, %d [%s:%d]-[%s:%d] %d:%s\n", rlen,
				  in->fd,
				  in->localip, in->local_port,
				  in->remoteip, in->remote_port,
				  errno, strerror(errno));
			return -1;
		}
		buffcache->index += rlen;
		httpheadlen = http_heard_end(buffcache->data, buffcache->index);
	}

	memset(&in->info_http_head, 0, sizeof(in->info_http_head));

	if (httpheadlen > 0)
	{
		char *ptindex, *ptend;
		const char *end = buffcache->data + httpheadlen;
		int ret;
		int heard_index = 0;

		ptindex = buffcache->data;
		while (ptindex < end)
		{
			ptend = strchr(ptindex, '\n');
			if (ptend)
				ptend++;

			if (!ptend)
			{ //data error
				break;
			}
			linelen = ptend - ptindex;
			memcpy(cacheline, ptindex, linelen);

			cacheline[linelen] = 0;
			printf("%s", cacheline);
			if (heard_index == 0)
			{
				if (strstr(cacheline, "/onvif/"))
				{
					in->info_http_head.isonvif_head = 1;
				}
				else
				{
					in->info_http_head.isonvif_head = 0;
				}
			}

			if (strncasecmp(cacheline, "Host:", 5) == 0)
			{
				snprintf(cacheline, sizeof(cacheline), "Host: %s:%d\r\n", out->remoteip, out->remote_port);
				linelen = strlen(cacheline);
				printf(">change>%s", cacheline);
			}
			else
			{ //Referer: http://9999999999:8082/mp4/images/css/
				//if (strncasecmp(cacheline, "Referer:", strlen("Referer:")) == 0)

				char buff1[20];
				char buff2[20];
				char *tmpstr;
				sprintf(buff1, "%s:%d", in->localip, in->local_port);
				if (out->remote_port != 80)
					sprintf(buff2, "%s:%d", out->remoteip, out->remote_port);
				else
					sprintf(buff2, "%s", out->remoteip);

				if (!strstr(cacheline, buff1))
				{
					sprintf(buff1, "%s", in->localip);
				}

				if (strstr(cacheline, buff1))
				{
					tmpstr = str_replace_new(cacheline, buff1, buff2);
					strcpy(cacheline, tmpstr);
					free(tmpstr);
					printf(">change[%s]->[%s]>[%s]", buff1, buff2, cacheline);
				}
			}

			ret = sockstream_send(out, cacheline, linelen);

			if (ptindex[0] == '\n' || (ptindex[0] == '\r' && ptindex[1] == '\n'))
			{
				break;
			}
			ptindex = ptend;
			heard_index++;
		}

		buffcache->index -= httpheadlen;
		memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);
		printf("-------other:%d----\n", buffcache->index);
	}
	return 0;
}

int _loop_http_head_response_in_out(struct sockstream *in, struct sockstream *out, struct buffdata *buffcache)
{
	int httpheadlen;
	char cacheline[1024];
	int linelen;
	int rlen = 0;

	httpheadlen = http_heard_end(buffcache->data, buffcache->index);
	while (httpheadlen == -1)
	{
		rlen = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index); //预留100字节做备份

		if (rlen <= 0)
		{
			if (rlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
			{
				usleep(1000);
				continue;
			}

			log_d("recv err, rlen=%d, %d [%s:%d]-[%s:%d] %d:%s\n", rlen,
				  in->fd,
				  in->localip, in->local_port,
				  in->remoteip, in->remote_port,
				  errno, strerror(errno));
			return -1;
		}
		buffcache->index += rlen;
		httpheadlen = http_heard_end(buffcache->data, buffcache->index);
	}

	memset(&in->info_http_head, 0, sizeof(in->info_http_head));

	if (httpheadlen > 0)
	{
		char *ptindex, *ptend;
		const char *end = buffcache->data + httpheadlen;
		int ret;

		ptindex = buffcache->data;
		while (ptindex < end)
		{
			ptend = strchr(ptindex, '\n');
			if (ptend)
				ptend++;

			if (!ptend)
			{ //data error
				break;
			}
			linelen = ptend - ptindex;
			memcpy(cacheline, ptindex, linelen);
			cacheline[linelen] = 0;
			printf("%s", cacheline);

			if (strncasecmp(cacheline, "Content-Length:", strlen("Content-Length:")) == 0)
			{
				in->info_http_head.contentLength = atoi(cacheline + strlen("Content-Length:"));
			}
			if (strncasecmp(cacheline, "Host:", 5) == 0)
			{
				snprintf(cacheline, sizeof(cacheline), "Host: %s:%d\r\n", out->localip, out->local_port);
				linelen = strlen(cacheline);
				printf(">change>%s", cacheline);
			}

			ret = sockstream_send(out, cacheline, linelen);

			if (ptindex[0] == '\n' || (ptindex[0] == '\r' && ptindex[1] == '\n'))
			{
				break;
			}
			ptindex = ptend;
		}

		buffcache->index -= httpheadlen;
		memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);
		printf("-------other:%d----\n", buffcache->index);
		printf("ContentLength:%d\n", in->info_http_head.contentLength);
	}
	return 0;
}

void *thread_run(void *arg)
{
	struct repeater_stream *repstream = (struct repeater_stream *)arg;
	pthread_detach(pthread_self());

	struct buffdata buffcache;
	int slen;
	int rlen;
	int ret;
	int tcp_nodelay = 1;

	char pname[16];
	sprintf(pname, "T:%d<->%d", repstream->instream.fd, repstream->outstream.fd);
	prctl(PR_SET_NAME, pname);

	buffcache.index = 0;
	buffcache.size = sizeof(buffcache.data);

	repeater_stream_reg(repstream);

	setsockopt(repstream->instream.fd, IPPROTO_TCP, TCP_NODELAY, (const void *)&tcp_nodelay,
			   sizeof(int));
	setsockopt(repstream->outstream.fd,
			   IPPROTO_TCP, TCP_NODELAY, (const void *)&tcp_nodelay, sizeof(int));

	socket_nonblock(repstream->instream.fd, 1);
	socket_nonblock(repstream->outstream.fd, 1);

	log_d("start thread.....\n");
	struct pollfd pfdreads[2];
	int loopflag = 1;

	int i;
	char *lineend = 0;
	char *ptr = NULL;
	int protocol_head = 0;

	struct sockstream *streams[2];
	struct sockstream *_streamin, *_streamout;
	streams[0] = &repstream->instream;
	streams[1] = &repstream->outstream;

	//net.ipv4.tcp_tw_recycle = 1

	while (loopflag)
	{
		memset(pfdreads, 0, sizeof(pfdreads));

		pfdreads[0].fd = streams[0]->fd;
		pfdreads[1].fd = streams[1]->fd;
		pfdreads[0].events = POLLIN;
		pfdreads[1].events = POLLIN;

		ret = poll(pfdreads, 2, 200);
		if (ret == 0)
			continue;

		if (ret < 0)
		{
			log_d("poll err:%d:%s\n", errno, strerror(errno));
			break;
		}

		for (i = 0; i < 2; i++)
		{
			if (pfdreads[i].revents & (POLLERR | POLLNVAL | POLLHUP))
			{
				log_d("POLLERR\n");
				loopflag = 0;
				break;
			}
		}
		if (!loopflag)
			break;
		//loop_handle_read_write
		////////////////////////////////////////////////////////////////
		for (i = 0; i < 2; i++)
		{
			if (0 == (pfdreads[i].revents & POLLIN))
				continue;
			_streamin = streams[i];
			_streamout = i == 0 ? streams[1] : streams[0];

			/////////////recv data///
			{
				rlen = sockstream_recv(streams[i], buffcache.data + buffcache.index, buffcache.size - buffcache.index - 100); //预留100字节做备份

				if (rlen <= 0)
				{
					if (rlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
					{
						continue;
					}

					loopflag = 0;
					log_d("recv err, rlen=%d, %d [%s:%d]-[%s:%d] %d:%s\n", rlen,
						  streams[i]->fd,
						  streams[i]->localip, streams[i]->local_port,
						  streams[i]->remoteip, streams[i]->remote_port,
						  errno, strerror(errno));
					break;
				}
				buffcache.index += rlen;
#if 1
				//是首次接收怎么办?
				if (!_streamin->isfirst)
				{
					if (rlen > 7)
					{
						buffcache.data[buffcache.index] = 0;
						if (ishttprequest(buffcache.data, buffcache.index))
						{
							_streamin->protocol = PROTO_TYPE_HTTP_REQUEST;
							log_d("recv ========= http request\n");
							sockstream_debug_print(_streamin);
							//is onvif http

							ret = _loop_http_head_request_in_out(_streamin, _streamout, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}
						}
						if (ishttpresponse(buffcache.data, buffcache.index))
						{
							_streamin->protocol = PROTO_TYPE_HTTP_RESPONSE;
							log_d("recv ========= http response\n");
							sockstream_debug_print(_streamin);

							ret = _loop_http_head_response_in_out(_streamin, _streamout, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}
						}
					}
					_streamin->isfirst = 1;
				}
#endif
			} ///end recv
			//printf("read len :%d \n", buffcache.index);
			//剩下的为内容替换
			if (buffcache.index == 0)
				continue;
			//收到数据后
			//需要转发给另一个,如果另一个一直阻塞呢?需要判断下一个http头哦
			//替换数据
			buffcache.data[buffcache.index] = 0;
			//如果是来自http相应数据
			//in
			if (_streamin->type == 0 && _streamin->protocol == PROTO_TYPE_HTTP_REQUEST)
			{
				//需要进行字符串替换,如果是 //需要替换这个IP地址
				char repstr1[20];
				char repstr2[20];

				strcpy(repstr1, _streamin->localip);
				sprintf(repstr2, "%s:%d", _streamin->localip, _streamin->local_port);

				if (strstr(buffcache.data, repstr2))
				{
					strcpy(repstr1, repstr2);
					sprintf(repstr2, "%s:%d", _streamout->remoteip, _streamout->remote_port);
				}
				else if (strstr(buffcache.data, repstr1))
				{
					sprintf(repstr2, "%s", _streamout->remoteip);
				}
				printf("request need replace str:[%s]->[%s]\n", repstr1, repstr2);
			}
			//out
			if (_streamin->type == 1 && _streamin->protocol == PROTO_TYPE_HTTP_RESPONSE)
			{
				//需要进行字符串替换,如果是
				char repstr1[20];
				char repstr2[20];

				strcpy(repstr1, _streamin->remoteip);
				sprintf(repstr2, "%s:%d", _streamin->remoteip, _streamin->remote_port);

				if (strstr(buffcache.data, repstr2))
				{
					strcpy(repstr1, repstr2);
					sprintf(repstr2, "%s:%d", _streamout->localip, _streamout->local_port);
				}
				else if (strstr(buffcache.data, repstr1))
				{
					sprintf(repstr2, "%s", _streamout->localip);
				}

				printf("response need replace str:[%s]->[%s]\n", repstr1, repstr2);
			}

			printf("%d->%d, len=%d\n", _streamin->fd, _streamout->fd, buffcache.index);

			struct pollfd pfdout[2];

			slen = 0;
			while (buffcache.index)
			{
				memset(pfdout, 0, sizeof(pfdout));
				pfdout[0].fd = _streamout->fd;
				pfdout[0].events = POLLOUT;
				pfdout[1].fd = _streamin->fd;
				pfdout[1].events = POLLERR;

				ret = poll(pfdout, 1, 200);
				if (ret == 0) //如果超时呢?
					continue;

				if (ret < 0)
				{
					slen = -1;
					break;
				}
				//
				if (pfdout[0].revents & (POLLERR | POLLNVAL | POLLHUP))
				{
					slen = -1;
					log_d("poll revent err:%d\n", pfdout[0].fd);
					break;
				}
				//
				if (pfdout[1].revents & (POLLERR | POLLNVAL | POLLHUP))
				{
					slen = -1;
					log_d("poll revent err:%d\n", pfdout[1].fd);
					break;
				}
				//
				if (pfdout[0].revents & POLLOUT)
				{
					ret = sockstream_send(_streamout, buffcache.data + slen, buffcache.index);
					if (ret <= 0)
					{
						if (ret != 0 && (errno == EAGAIN || errno == EINTR))
						{
							printf("out is EAGAIN ...\n");
							continue;
						}
						slen = -1;
						break;
					}
					buffcache.index -= ret;
					slen += ret;
					//printf("send len:%d \n", ret);
				}
			}

			if (buffcache.index != 0)
			{
				printf("转发错误\n");
			}
			if (slen == -1)
			{
				log_d("send err,%d[%d]\n", _streamout->fd, buffcache.index);
				break;
			}
		}
		//////////////////////////////////////
	}

	repeater_stream_unreg(repstream);

	struct linger linger;
	linger.l_onoff = 1;  //0=off, nonzero=on(开关)
	linger.l_linger = 0; //linger time(延迟时间)

	setsockopt(repstream->instream.fd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
			   sizeof(struct linger));

	setsockopt(repstream->outstream.fd, SOL_SOCKET, SO_LINGER, (const void *)&linger,
			   sizeof(struct linger));

	shutdown(repstream->instream.fd, SHUT_RDWR);
	close(repstream->instream.fd);

	shutdown(repstream->outstream.fd, SHUT_RDWR);
	close(repstream->outstream.fd);

	log_d("close....\n");
	free(repstream);
	return 0;
}

int epoll_add(int epfd, int fd, int events, void *ptr)
{
	struct epoll_event event;
	event.events = events;
	event.data.ptr = ptr;
	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
}

void repeater_dispatcher()
{
	int epfd = epoll_create1(0);
	int ret;
	int i = 0;

	struct repeater_server repserver;

	{

		//repeater_server_init(&repserver, "0.0.0.0", 8082, "14.215.177.38", 80);
		repeater_server_init(&repserver, "0.0.0.0", 8082, "192.168.8.106", 8081);

		repserver.fd = socket_bind_tcp(repserver.serveraddr.ipstr, repserver.serveraddr.port, 1, 1);
		if (repserver.fd == -1)
		{
			perror("not create \n");
		}
		else
		{
			epoll_add(epfd, repserver.fd, EPOLLIN, &repserver);
		}
	}

	struct epoll_event events[100];
	struct repeater_stream *stream;
	int addrlen;
	int eplen;
	int fdin, fdout;
	struct sockaddr_in inaddr;

	while (1)
	{
		eplen = epoll_wait(epfd, events, 100, 100);
		if (eplen == 0)
			continue;
		if (eplen < 0)
		{
			perror("epoll wait..");
			if (errno == EAGAIN || errno == EINTR)
				continue;
			break;
		}
		log_d("next..\n", time(NULL));
		for (i = 0; i < eplen; i++)
		{
			if (events[i].data.ptr == &repserver)
			{
				if (events[i].events & EPOLLIN == EPOLLIN)
				{
					addrlen = sizeof(inaddr);
					fdin = accept(repserver.fd, (struct sockaddr *)&inaddr, &addrlen);
					if (fdin <= 0)
					{
						continue;
					}

					fdout = socket_connect(repserver.toaddr.ipstr, repserver.toaddr.port);
					if (fdout == -1)
					{
						printf("not connect to %s\n", repserver.toaddr.ipstr);
						close(fdin);
						continue;
					}

					stream = (struct repeater_stream *)calloc(sizeof(typeof(*stream)), 1);

					memcpy(&stream->repserver, &repserver, sizeof(repserver));

					stream->instream.type = 0;
					stream->instream.fd = fdin;
					memcpy(&stream->instream.addrremote, &inaddr, sizeof(inaddr));
					addrlen = sizeof(struct sockaddr_in);
					getsockname(fdin, (struct sockaddr *)&stream->instream.addrlocal, &addrlen);

					inet_ntop(AF_INET, &stream->instream.addrlocal.sin_addr,
							  stream->instream.localip, 20);
					inet_ntop(AF_INET, &stream->instream.addrremote.sin_addr,
							  stream->instream.remoteip, 20);

					stream->instream.local_port = ntohs(stream->instream.addrlocal.sin_port);
					stream->instream.remote_port = ntohs(stream->instream.addrremote.sin_port);

					strcpy(stream->repserver.serveraddr.ipstr, stream->instream.localip);

					stream->outstream.type = 1;
					stream->outstream.fd = fdout;
					addrlen = sizeof(struct sockaddr_in);
					getsockname(fdout, (struct sockaddr *)&stream->outstream.addrlocal, &addrlen);
					addrlen = sizeof(struct sockaddr_in);
					getpeername(fdout, (struct sockaddr *)&stream->outstream.addrremote, &addrlen);

					inet_ntop(AF_INET, &stream->outstream.addrlocal.sin_addr,
							  stream->outstream.localip, 20);
					inet_ntop(AF_INET, &stream->outstream.addrremote.sin_addr,
							  stream->outstream.remoteip, 20);
					stream->outstream.local_port = ntohs(stream->outstream.addrlocal.sin_port);
					stream->outstream.remote_port = ntohs(stream->outstream.addrremote.sin_port);

					repeater_stream_debug_print(stream);

					pthread_t pt;
					pthread_create(&pt, NULL, thread_run, stream);
				}
				continue;
			}

			stream = (typeof(stream))(events[i].data.ptr);
		}
	}
	close(epfd);
}

int main(int argc, const char **argv)
{
	//signal(SIGINT, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	char *src = "GET /mp4/images/ HTTP/1.1\r\n"
				"Host: 127.0.0.1:8082\r\n"
				"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0"
				"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
				"Accept-Language: zh-CN,en-US;q=0.7,en;q=0.3\r\n"
				"Accept-Encoding: gzip, deflate\r\n"
				"Referer: http://127.0.0.1:8082/mp4/images/css/\r\n"
				"Connection: keep-alive\r\n"
				"Cookie: BD_UPN=133352; H_PS_645EC=69b1RTU5Wl2O08cV%2FB9eRy8VjT39d4D7H2YX9veNW3ouiTje5VcxDvfIAG0\r\n"
				"Upgrade-Insecure-Requests: 1\r\n"
				"\r\n0xFFFFF";
	char buff[500];
	int blen = 0;

	str_replace(src, "127.0.0.1", "9999999999", buff, sizeof(buff), &blen);
	printf("%s", buff);
	printf("len=%ld, blen=%d\n", strlen(buff), blen);

	{
		char *ptr = str_replace_new(src, "0xFFFFF", "9999999999");
		printf("==============================%ld==\n", strlen(ptr));
		printf("%s", ptr);
		printf("==========================\n");
		free(ptr);
	}

	repeater_dispatcher();
	return 0;
}
