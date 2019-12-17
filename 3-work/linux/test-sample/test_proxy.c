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

#define strncasecmpex(s1, s2) strncasecmp(s1, s2, strlen(s2))
#define strncmpex(s1, s2) strncmp(s1, s2, strlen(s2))

char *ex_strndup(const char *buf, size_t n)
{
	char *str = (char *)malloc(n + 1);
	if (str)
	{
		str[n] = 0;
		strncpy(str, buf, n);
	}
	return str;
}

/* 功  能：将str字符串中的oldstr字符串替换为newstr字符串
 * 参  数：str：操作目标 oldstr：被替换者 newstr：替换者
 * 返回值：返回替换之后的字符串
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

int str_replace_(char *str, const char *strold, const char *strnew)
{
	char *ptr = str;
	int stroldlen = strlen(strold);
	if (stroldlen != strlen(strnew))
		return -1;

	while (*ptr)
	{
		if (strncmp(ptr, strold, stroldlen) == 0)
		{
			memcpy(ptr, strnew, stroldlen);
			ptr += stroldlen;
			continue;
		}
		ptr++;
	}
	return 0;
}

char *str_replace_r(const char *src, const char *strold, const char *strnew, char *strto, int tolen, int *pnewlen)
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
char *str_replace(const char *str, const char *src, const char *dst)
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

int string_getline(const char *str, char *linestr, int linesize)
{
	const char *ptend = strchr(str, '\n');
	int linelen = 0;
	if (!ptend)
		return -1;
	ptend++;

	if (ptend - str >= linesize)
	{
		ptend = str + linesize - 1;
	}

	linelen = ptend - str;
	memcpy(linestr, str, linelen);
	linestr[linelen] = 0;
	return linelen;
}

int logprintf(const char *file, const char *function, int line, const char *format, ...)
{
	char tmstr[20];
	struct tm tm;
	struct timeval tp;
	const char *filename = strrchr(file, '/');
	if (filename)
		filename++;
	else
		filename = file;

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
	char ethName[30];
	char ipstr[20];
	//int fd;
	int port;
	int port_rtsp;
};

//转发服务
struct repeater_server
{
	struct addrinfo serveraddr; //转发器服务地址
	struct addrinfo toaddr;		//被转发到地址
	int fd;

	char description[80];
};

void repeater_server_description_init(struct repeater_server *sitem)
{
	snprintf(sitem->description, sizeof(sitem->description),
			 "fd:%d [%s:%s:%d -> %s:%s:%d]",
			 sitem->fd,
			 sitem->serveraddr.ethName, sitem->serveraddr.ipstr, sitem->serveraddr.port,
			 sitem->toaddr.ethName, sitem->toaddr.ipstr, sitem->toaddr.port);
}

enum protocoltype
{
	PROTO_TYPE_NONE,
	PROTO_TYPE_HTTP_REQUEST,
	PROTO_TYPE_HTTP_RESPONSE
	//PROTO_TYPE_HTTP_REQ_ONVIF
};

typedef enum http_cmd_type
{
	HTTP_NONE = 0,
	HTTP_REQ_GET = 1 << 0,
	HTTP_REQ_POST = 1 << 1,
	HTTP_REQ_HEAD = 1 << 2,
	HTTP_REQ_PUT = 1 << 3,
	HTTP_REQ_DELETE = 1 << 4,
	HTTP_REQ_OPTIONS = 1 << 5,
	HTTP_REQ_TRACE = 1 << 6,
	HTTP_REQ_CONNECT = 1 << 7,
	HTTP_REQ_PATCH = 1 << 8
} http_req_type;

http_req_type http_req_gettype(const char *httphead)
{
	if (strncmpex(httphead, "GET ") == 0)
		return HTTP_REQ_GET;
	if (strncmpex(httphead, "POST ") == 0)
		return HTTP_REQ_POST;
	if (strncmpex(httphead, "HEAD ") == 0)
		return HTTP_REQ_HEAD;
	if (strncmpex(httphead, "PUT ") == 0)
		return HTTP_REQ_PUT;
	if (strncmpex(httphead, "DELETE ") == 0)
		return HTTP_REQ_DELETE;
	if (strncmpex(httphead, "OPTIONS ") == 0)
		return HTTP_REQ_OPTIONS;
	if (strncmpex(httphead, "TRACE ") == 0)
		return HTTP_REQ_TRACE;
	if (strncmpex(httphead, "CONNECT ") == 0)
		return HTTP_REQ_CONNECT;
	if (strncmpex(httphead, "PATCH ") == 0)
		return HTTP_REQ_PATCH;
	return HTTP_NONE;
}

int ishttprequest(char *buff, int len)
{
	return http_req_gettype(buff);
}

int ishttpresponse(char *buff, int len)
{
	if (strncasecmp(buff, "HTTP/", 5) == 0)
	{
		return 1;
	}
	return 0;
}

//查找http头长度
int http_heard_getlength(const char *buf, int len)
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

struct proto_http_info
{
	http_req_type http_method; //http请求方法 GET POST PUT ...
	int http_response_code;	//http响应码 在响应中有效

	int path_is_onvif;			  //
	int path_is_cgi_bin_main_cgi; ///cgi-bin/main-cgi

	int content_length; //内容长度
	int iskeep_alive;
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

	char description[100];
	enum protocoltype protocol; //0=unknow, 1=http
	struct proto_http_info info_http_head;

	int tm_lastr; //最后接收时间
	int tm_lastw; //最后发送时间
	int isfirst;  //首次接收情况
};

//替换此数据流
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
	log_d("%s\n", stream->description);
	return 0;
}

int repeater_stream_debug_print(struct repeater_stream *repstream)
{
	log_d("-------------------------------\n");
	log_d("%s <-> %s\n", repstream->instream.description, repstream->outstream.description);
	log_d("-------------------------------\n");
	return 0;
}

char *string_replace_ipinfo(const char *buff, const char *str_oldip, int port_old, const char *str_newip, int port_new)
{
	char str1[23]; //192.168.200.189:65535
	char str2[23];
	char *str = NULL;

	snprintf(str1, sizeof(str1), "%s:%d", str_oldip, port_old);
	snprintf(str2, sizeof(str1), "%s:%d", str_newip, port_new);

	if (!strstr(buff, str1))
	{
		snprintf(str1, sizeof(str1), "%s", str_oldip);
		snprintf(str2, sizeof(str1), "%s", str_newip);
	}

	if (strstr(buff, str1))
		str = str_replace(buff, str1, str2);

	return str;
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

void repeater_server_init(struct repeater_server *item,
						  const char *server_ip, int server_port, int server_port_rtsp,
						  const char *dst_ip, int dst_port, int dst_port_rtsp)
{
	strcpy(item->serveraddr.ipstr, server_ip);
	item->serveraddr.port = server_port;
	item->serveraddr.port_rtsp = server_port_rtsp;
	strcpy(item->toaddr.ipstr, dst_ip);
	item->toaddr.port = dst_port;
	item->toaddr.port_rtsp = dst_port_rtsp;
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
				usleep(1000);
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

int poll_isin(int fd)
{
	struct pollfd pfd;
	int ret;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	ret = poll(&pfd, 1, 0);
	return pfd.revents & POLLIN == POLLIN ? 1 : 0;
}

int poll_isout(int fd)
{
	struct pollfd pfd;
	int ret;
	pfd.fd = fd;
	pfd.events = POLLOUT;
	pfd.revents = 0;
	ret = poll(&pfd, 1, 0);
	return pfd.revents & POLLOUT == POLLOUT ? 1 : 0;
}

struct buffdata
{
	char data[1024 * 20]; //20K=20480, 13926
	int index;
	int size;
};

char *buffdata_split_http_headstr(struct buffdata *buf)
{
	char *str = NULL;
	int length = http_heard_getlength(buf->data, buf->index);
	if (length > 0)
	{
		str = (char *)malloc(length + 1);
		if (!str)
			return NULL;
		strncpy(str, buf->data, length);
		str[length] = 0;
		//str = strndup(buf->data, length);
		buf->index -= length;
		memcpy(buf->data, buf->data + length, buf->index);
		return str;
	}
	return str;
}

int _loop_http_head_read(struct sockstream *in, struct buffdata *buffcache)
{
	int httpheadlen;
	int ret;
	httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);
	while (httpheadlen == -1)
	{
		ret = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index); //预留100字节做备份

		if (ret <= 0)
		{
			if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
			{
				usleep(1000);
				continue;
			}

			log_d("recv err, ret=%d, %d [%s:%d]-[%s:%d] %d:%s\n", ret,
				  in->fd,
				  in->localip, in->local_port,
				  in->remoteip, in->remote_port,
				  errno, strerror(errno));
			return -1;
		}
		buffcache->index += ret;
		httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);
	}
	return 0;
}

int _loop_http_head_request_in_out(struct repeater_stream *repstream,
								   struct sockstream *in,
								   struct sockstream *out,
								   struct buffdata *buffcache)
{
	int httpheadlen;
	char cacheline[1024];
	int linelen;
	int rlen = 0;

	httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);
	memset(&in->info_http_head, 0, sizeof(in->info_http_head));

	if (httpheadlen <= 0)
		return 0;

	{
		char *ptindex, *ptend;
		const char *end = buffcache->data + httpheadlen - 1;
		int ret;
		int heard_index = 0;

		ptindex = buffcache->data;
		in->info_http_head.http_method = http_req_gettype(buffcache->data);
		while (ptindex < end)
		{
			ret = string_getline(ptindex, cacheline, sizeof(cacheline));

			ptindex += ret;
			log_d("%s", cacheline);

			if (heard_index == 0)
			{
				if (strstr(cacheline, "/onvif/"))
					in->info_http_head.path_is_onvif = 1;

				if (strstr(cacheline, "/cgi-bin/main-cgi"))
					in->info_http_head.path_is_cgi_bin_main_cgi = 1;
			}

			{ //IP替换
				char *tmpstr = string_replace_ipinfo(cacheline, in->localip, in->local_port, out->remoteip, out->remote_port);
				if (tmpstr)
				{
					strcpy(cacheline, tmpstr);
					free(tmpstr);
					log_d(">change->[%s]", cacheline);
				}
			}

			ret = sockstream_send(out, cacheline, strlen(cacheline));
			heard_index++;
		}

		buffcache->index -= httpheadlen;
		memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);
		log_d("-------other:%d----\n", buffcache->index);

		if (in->info_http_head.path_is_cgi_bin_main_cgi && in->info_http_head.content_length > 0)
		{
			if (in->info_http_head.content_length < buffcache->size - 100)
			{
				int need_length = in->info_http_head.content_length - buffcache->index;
				int rlen = 0;
				while (need_length > 0)
				{
					ret = sockstream_recv(in, buffcache->data + buffcache->index, need_length);
					if (ret <= 0)
					{
						if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
						{
							usleep(1000);
							continue;
						}

						log_d("recv err, ret=%d, %d [%s:%d]-[%s:%d] %d:%s\n", ret,
							  in->fd,
							  in->localip, in->local_port,
							  in->remoteip, in->remote_port,
							  errno, strerror(errno));
						rlen = -1;
						break;
					}
					buffcache->index += ret;
					rlen += ret;
					need_length -= ret;
				}
				if (rlen == -1)
					return -1;

				buffcache->data[buffcache->index] = 0;

				if (in->info_http_head.content_length == buffcache->index)
				{
					in->isfirst = 0;
				}

				log_d("is_cgi_bin_main_cgi>%s\n", buffcache->data);
			}
		}
	}
	return 0;
}

int _loop_http_response_nvr(struct repeater_stream *repstream,
							struct sockstream *in,
							struct sockstream *out,
							struct buffdata *buffcache)
{
	int ret = 0;
	char *http_heard = NULL;
	char *ptr, *ptr_end;

	int httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);
	if (httpheadlen == -1)
	{
		log_d("not read http head \n");
		return -1;
	}
	log_d("nvr handle\n");
	http_heard = ex_strndup(buffcache->data, httpheadlen);
	if (!http_heard)
		return -1;

	buffcache->index -= httpheadlen;
	memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);

	int rlencount = 0;

	if (in->info_http_head.content_length > 0)
	{
	}
	while (buffcache->size - buffcache->index - 500 > 0)
	{ //20K数据量处理
		ret = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index - 500);
		if (ret <= 0)
		{
			if (ret < 0 && (errno == EAGAIN || errno == EINTR))
			{
				usleep(1000); //1ms
				rlencount++;
				if (rlencount > 1000 * 2)
				{
					break;
				}
				continue;
			}
			ret = -1;
			break;
		}
		rlencount = 0;
		buffcache->index += ret;
		ret = 0;
		if (in->info_http_head.content_length > 0 && buffcache->index == in->info_http_head.content_length)
		{
			break;
		}
	}
	buffcache->data[buffcache->index] = 0;

	// {
	//  "u32Task_No": 241888135,
	//  "u8RecvSendFlag": 1,
	//  "u16Port": 80,
	//  "stIPAddress": "192.168.0.198",
	//  "szSessionID": "ldq10054",
	//  "u16PayloadCount": 1,
	//  "astPayloadTypeIE": [{
	//    "u16Type": 2,
	//    "u16PtVal": 96
	//   }],
	//  "code": 0,
	//  "success": true
	// }
	char str1[20];
	char str2[20];

	snprintf(str1, sizeof(str1), "\"%s\"", in->remoteip);
	snprintf(str2, sizeof(str2), "\"%s\"", out->localip);

	if (strstr(buffcache->data, str1))
	{
		char *tmpstr = str_replace(buffcache->data, str1, str2);
		log_d("replace ip:[%s]->[%s]\n", str1, str2);
		strcpy(buffcache->data, tmpstr);
		buffcache->index = strlen(tmpstr);
		free(tmpstr);
		ret = 0;
	}
	if (in->info_http_head.content_length > 0)
	{
		//转发数据内容的时候的处理
	}
	//需要转发
	ret = socket_send(out->fd, http_heard, strlen(http_heard));
	if (ret != -1)
		ret = 0;
	//socket_send(out->fd, buffcache->data, buffcache->index);
	free(http_heard);
	return ret;
}

int _loop_http_response_onvif(struct repeater_stream *repstream,
							  struct sockstream *in,
							  struct sockstream *out,
							  struct buffdata *buffcache)
{
	int ret;
	char *http_heard = NULL;
	int httpheadlen = 0;
	char *strhead;
	const char *end;
	char cacheline[2024];
	int linelen;
	int http_response_code;

	{ //分离http头数据
		httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);
		if (httpheadlen == -1)
		{
			log_d("onvif response not read http head \n");
			return -1;
		}
		strhead = buffcache->data;
		end = buffcache->data + httpheadlen;

		http_heard = ex_strndup(buffcache->data, httpheadlen);
		if (!http_heard)
		{
			log_d("onvif not split http heard\n");
			return -1;
		}
		// http头处理
		// while (strhead < end)
		// {
		// 	ret = string_getline(strhead, cacheline, sizeof(cacheline));
		// 	strhead += ret;
		// }
		buffcache->index -= httpheadlen;
		memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);
		log_d("-------other:%d------\n", buffcache->index);
		log_d("Content-Length:%d\n", in->info_http_head.content_length);
	}

	char *xmlstring = NULL;
	int xmllength = 0;

	if (in->info_http_head.content_length > 0) //当Content-Length存在的时候
	{
		xmlstring = (char *)malloc(in->info_http_head.content_length + 1);
		memcpy(xmlstring, buffcache->data, buffcache->index);
		xmlstring[in->info_http_head.content_length] = 0;
		xmllength = buffcache->index;
		buffcache->index = 0;

		log_d(" xmlength cur=%d\n", xmllength);

		if (xmllength < in->info_http_head.content_length)
		{
			while (xmllength < in->info_http_head.content_length)
			{
				ret = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index); //预留100字节做备份

				if (ret <= 0)
				{
					if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
					{
						usleep(1000);
						continue;
					}

					log_d("recv err, ret=%d, %d [%s:%d]-[%s:%d] %d:%s\n", ret,
						  in->fd,
						  in->localip, in->local_port,
						  in->remoteip, in->remote_port,
						  errno, strerror(errno));
					break;
				}

				memcpy(xmlstring + xmllength, buffcache->data, ret);
				xmllength += ret;
				xmlstring[xmllength] = 0;
				printf(" read sum=%d\n", xmllength);
			}
		}

		if (xmllength < in->info_http_head.content_length)
		{
			ret = -1;
			printf("read http context err\n");
			goto _return;
		}

		char buff1[30]; //http://%s:%d  http://255.255.255.255:65535(len=28)
		char buff2[30];

		//rtsp地址替换
		if (repstream->repserver.serveraddr.port_rtsp)
		{
			snprintf(buff1, sizeof(buff1), "rtsp://%s:%d/", in->remoteip,
					 repstream->repserver.toaddr.port_rtsp);

			snprintf(buff2, sizeof(buff2), "rtsp://%s:%d/", out->localip,
					 repstream->repserver.serveraddr.port_rtsp);

			if (strstr(xmlstring, buff1))
			{
				log_d("replace [%s]->[%s]\n", buff1, buff2);
				char *tmpstr = str_replace(xmlstring, buff1, buff2);
				if (tmpstr)
				{
					free(xmlstring);
					xmlstring = tmpstr;
				}
			}
		}

		//http 端口替换
		snprintf(buff1, sizeof(buff1), "http://%s:%d/", in->remoteip, in->remote_port);
		snprintf(buff2, sizeof(buff2), "http://%s:%d/", out->localip, out->local_port);

		if (!strstr(xmlstring, buff1))
		{
			snprintf(buff1, sizeof(buff1), "http://%s/", in->remoteip);
			if (out->local_port != 80)
			{
				snprintf(buff2, sizeof(buff2), "http://%s:%d/", out->localip, out->local_port);
			}
			else
			{
				snprintf(buff2, sizeof(buff2), "http://%s/", out->localip);
			}
		}
		if (strstr(xmlstring, buff1))
		{
			log_d("replace [%s]->[%s]\n", buff1, buff2);

			char *tmpstr = str_replace(xmlstring, buff1, buff2);
			if (tmpstr)
			{
				free(xmlstring);
				xmlstring = tmpstr;
			}
		}

		xmllength = strlen(xmlstring);
		//GetStreamUriResponse 中的 rtsp 端口替换
		strhead = http_heard;
		end = http_heard + httpheadlen;

		//printf("%s", http_heard, httpheadlen);
		while (strhead < end)
		{
			ret = string_getline(strhead, cacheline, sizeof(cacheline));
			strhead += ret;
			log_d("%s", cacheline);

			if (strncasecmp(cacheline, "Content-Length:", strlen("Content-Length:")) == 0)
			{
				snprintf(cacheline, sizeof(cacheline), "Content-Length: %d\r\n", xmllength);
			}
			socket_send(out->fd, cacheline, strlen(cacheline));
		}

		if (xmllength > 50)
			log_d("%.50s ...\n", xmlstring);
		ret = socket_send(out->fd, xmlstring, strlen(xmlstring));
	} //in content_length>0
	else
	{
		//需要判断如果是200呢?怎么办?
		log_d("http_heard>>>\n%s", http_heard);
		ret = socket_send(out->fd, http_heard, httpheadlen);

		if (http_response_code == 200)
		{
			//需要替换数据
			int recvcount = 0;
			int ret_r = 0;
			int ret_s = 0;
			while (1)
			{
				if (buffcache->size - buffcache->index > 2)
				{
					ret_r = sockstream_recv(in, buffcache->data + buffcache->index, buffcache->size - buffcache->index);
					if (ret_r <= 0)
					{
						if (ret_r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
						{
							//如果一直在这里内? 3秒断开,这个时候输入是没有数据的
							if (poll_isin(out->fd))
							{
								break;
							}
							usleep(1000); //1s ==1000ms 1ms=1000us
							recvcount++;

							if (recvcount > 3 * 1000)
							{
								break;
							}
							continue;
						}
						//接收断开
						recvcount = 0;
					}
				}
				buffcache->data[buffcache->index] = 0;

				//ret = sockstream_send(out, buffcache->data, buffcache->index);
				ret_s = socket_send(out->fd, buffcache->data, buffcache->index);
				if (ret_s == -1)
					break;

				if (ret_r <= 0) //接收断开
					break;
				buffcache->index = 0;
			} //http 200 while end
			if (ret_s > 0 && ret_r > 0)
				ret = 0;
			else
				ret = -1;
		}
	}

_return:
	if (xmlstring)
		free(xmlstring);

	if (http_heard)
		free(http_heard);
	return ret;
}

int _loop_http_head_response_in_out(struct repeater_stream *repstream,
									struct sockstream *in,
									struct sockstream *out,
									struct buffdata *buffcache)
{
	int httpheadlen;
	int rlen = 0;
	int http_response_code = 0;

	httpheadlen = http_heard_getlength(buffcache->data, buffcache->index);

	memset(&in->info_http_head, 0, sizeof(in->info_http_head));
	if (httpheadlen <= 0)
		return 0;

	sscanf(buffcache->data, "%*[^ ] %d", &http_response_code);
	in->info_http_head.http_response_code = http_response_code;
	log_d("httpcode=%d\n", http_response_code); //

	//Content-Length:
	{
		char *ptr = buffcache->data;
		char *ptr_end = ptr + httpheadlen - 1;
		while (ptr < ptr_end)
		{
			if (0 == strncasecmpex(ptr, "Content-Length:"))
			{
				in->info_http_head.content_length = atoi(ptr + strlen("Content-Length:"));
			}
			ptr = strchr(ptr, '\n');
			if (!ptr)
				break;
			ptr++;
		}
	}
	log_d("Content-Length=%d\n", in->info_http_head.content_length);

	{
		if (out->info_http_head.path_is_onvif)
		{
			log_d("this is onvif http response\n");
			_loop_http_response_onvif(repstream, in, out, buffcache);
			return -1;
		}
		else if (out->info_http_head.path_is_cgi_bin_main_cgi)
		{
			log_d("this is _loop_http_response_nvr\n");
			return _loop_http_response_nvr(repstream, in, out, buffcache);
		}
		else
		{
			char cacheline[1024];
			int linelen;
			char *strhead;
			const char *end = buffcache->data + httpheadlen;
			int ret;
			int http_resp_code;

			strhead = buffcache->data;
			sscanf(strhead, "%*[^ ] %d", &http_resp_code);

			log_d("%s http响应:%d\n", in->description, http_resp_code);
			while (strhead < end)
			{
				ret = string_getline(strhead, cacheline, sizeof(cacheline));
				strhead += ret;

				if (strncasecmp(cacheline, "Content-Length:", strlen("Content-Length:")) == 0)
				{
					in->info_http_head.content_length = atoi(cacheline + strlen("Content-Length:"));
				}

				linelen = strlen(cacheline);
				ret = sockstream_send(out, cacheline, linelen);

				log_d("ret=%d>%s", ret, cacheline);
			}

			buffcache->index -= httpheadlen;
			memcpy(buffcache->data, buffcache->data + httpheadlen, buffcache->index);
			log_d("-------other:%d----\n", buffcache->index);
			log_d("Content-Length:%d\n", in->info_http_head.content_length);
		}
	}
	return 0;
}

void *thread_run(void *arg)
{
	struct repeater_stream *repstream = (struct repeater_stream *)arg;
	pthread_detach(pthread_self());

	{
		int addrlen;
		int fdout = socket_connect(repstream->repserver.toaddr.ipstr, repstream->repserver.toaddr.port);
		if (fdout == -1)
		{
			log_d("not connect to %s\n", repstream->repserver.toaddr.ipstr);
			close(repstream->instream.fd);
			free(repstream);
			return NULL;
		}
		//strcpy(stream->repserver.serveraddr.ipstr, stream->instream.localip);

		repstream->outstream.type = 1;
		repstream->outstream.fd = fdout;
		addrlen = sizeof(struct sockaddr_in);
		getsockname(fdout, (struct sockaddr *)&repstream->outstream.addrlocal, &addrlen);
		addrlen = sizeof(struct sockaddr_in);
		getpeername(fdout, (struct sockaddr *)&repstream->outstream.addrremote, &addrlen);

		inet_ntop(AF_INET, &repstream->outstream.addrlocal.sin_addr,
				  repstream->outstream.localip, 20);
		inet_ntop(AF_INET, &repstream->outstream.addrremote.sin_addr,
				  repstream->outstream.remoteip, 20);
		repstream->outstream.local_port = ntohs(repstream->outstream.addrlocal.sin_port);
		repstream->outstream.remote_port = ntohs(repstream->outstream.addrremote.sin_port);

		sprintf(repstream->instream.description, "src[%s:%d] fd:%d [%s:%d->%s:%d]",
				repstream->repserver.serveraddr.ipstr,
				repstream->repserver.serveraddr.port,
				repstream->instream.fd,
				repstream->instream.remoteip, repstream->instream.remote_port,
				repstream->instream.localip, repstream->instream.local_port);

		sprintf(repstream->outstream.description, "dst[%s:%d] fd:%d (%s:%d->%s:%d)",
				repstream->repserver.toaddr.ipstr,
				repstream->repserver.toaddr.port,
				repstream->outstream.fd,
				repstream->outstream.localip, repstream->outstream.local_port,
				repstream->outstream.remoteip, repstream->outstream.remote_port);

		repeater_stream_debug_print(repstream);
	}

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

			//  recv data //
			{
				rlen = sockstream_recv(streams[i], buffcache.data + buffcache.index, buffcache.size - buffcache.index - 100); //预留100字节做备份

				if (rlen <= 0)
				{
					if (rlen < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) //需要重新接收
					{
						continue;
					}

					loopflag = 0;
					log_d("recv err,ret=%d,%s,%d:%s\n", rlen,
						  streams[i]->description,
						  errno, strerror(errno));
					break;
				}
				buffcache.index += rlen;
#if 1

				buffcache.data[buffcache.index] = 0;

				if (_streamin->type == 0 && ishttprequest(buffcache.data, buffcache.index))
				{
					log_d("新的http请求 %s\n", _streamin->description); //XXX /path HTTP/?.?\r\n format
				}
				if (_streamin->type == 1 && ishttpresponse(buffcache.data, buffcache.index))
				{
					log_d("新的http响应 %s\n", _streamin->description); //HTTP/1.1 200 OK\r\n format
				}

				//是首次接收怎么办?
				if (!_streamin->isfirst)
				{
					_streamin->isfirst = 1;
					if (rlen > 7)
					{
						if (ishttprequest(buffcache.data, buffcache.index))
						{
							_streamin->protocol = PROTO_TYPE_HTTP_REQUEST;
							log_d("recv ========= http request %s\n", _streamin->description);
							//is onvif http
							ret = _loop_http_head_read(_streamin, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}

							ret = _loop_http_head_request_in_out(repstream, _streamin, _streamout, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}
						}
						if (ishttpresponse(buffcache.data, buffcache.index))
						{
							_streamin->protocol = PROTO_TYPE_HTTP_RESPONSE;
							log_d("recv ========= http response %s\n", _streamin->description);

							ret = _loop_http_head_read(_streamin, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}

							ret = _loop_http_head_response_in_out(repstream, _streamin, _streamout, &buffcache);
							if (ret == -1)
							{
								loopflag = 0;
								break;
							}
						}
					}
				}
#endif
			} ///end recv
			//printf("read len :%d \n", buffcache.index);
			//剩下的为内容替换
			if (buffcache.index == 0 || loopflag == 0)
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

				sprintf(repstr1, "%s:%d", _streamin->localip, _streamin->local_port);
				if (strstr(buffcache.data, repstr1))
				{
					sprintf(repstr1, "%s:%d", _streamout->remoteip, _streamout->remote_port);
				}
				else
				{
					strcpy(repstr1, _streamin->localip);
					if (strstr(buffcache.data, repstr1))
					{
						strcpy(repstr2, _streamout->remoteip);
					}
				}
				if (strstr(buffcache.data, repstr1))
				{
					log_d("request need replace str:[%s]->[%s]\n", repstr1, repstr2);
					if (strlen(repstr1) == strlen(repstr2))
					{
						str_replace_(buffcache.data, repstr1, repstr2);
					}
				}
			}
			//out
			if (_streamin->type == 1 && _streamin->protocol == PROTO_TYPE_HTTP_RESPONSE)
			{
				//需要进行字符串替换,如果是
				char repstr1[20];
				char repstr2[20];

				sprintf(repstr1, "%s:%d", _streamin->remoteip, _streamin->remote_port);

				if (strstr(buffcache.data, repstr1))
				{
					sprintf(repstr2, "%s:%d", _streamout->localip, _streamout->local_port);
				}
				else
				{
					strcpy(repstr1, _streamin->remoteip);
					if (strstr(buffcache.data, repstr1))
					{
						sprintf(repstr2, "%s", _streamout->localip);
					}
				}
				if (strstr(buffcache.data, repstr1))
				{
					log_d("response need replace str:[%s]->[%s]\n", repstr1, repstr2);
					if (strlen(repstr1) == strlen(repstr2)) //长度相等就替换
					{
						str_replace_(buffcache.data, repstr1, repstr2);
					}
				}
			}
			if (_streamin->protocol == PROTO_TYPE_HTTP_REQUEST ||
				_streamin->protocol == PROTO_TYPE_HTTP_RESPONSE)
			{
				printf("in2out: %s->%s, len=%d\n",
						_streamin->description,
						_streamout->description,
						buffcache.index);
				if (buffcache.index > 100)
					printf("%.100s ...\n", buffcache.data);
				else if(buffcache.index > 20)
					printf("%.20s ...\n", buffcache.data);
				else
					printf("%s\n ...", buffcache.data);
			}

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

					if (PROTO_TYPE_HTTP_REQUEST == _streamout->protocol ||
						PROTO_TYPE_HTTP_RESPONSE == _streamout->protocol)
						log_d(" ->>%s,%d\n", _streamout->description, ret);
				}
			}

			if (buffcache.index != 0)
			{
				log_d("转发错误\n");
			}
			if (slen == -1)
			{
				log_d("send err,%s[%d]\n", _streamout->description, buffcache.index);
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

void repeater_dispatcher(struct repeater_server *servers, int count)
{
	int epfd = epoll_create1(0);
	int ret;
	int i = 0;
	struct repeater_server *repseritem = NULL;

	for (i = 0; i < count; i++)
	{
		printf("server %d:%s\n", i + 1, servers[i].description);
	}
	for (i = 0; i < count; i++)
	{
		//log_d("start listen: %s\n", servers[i].description);

		servers[i].fd = socket_bind_tcp(servers[i].serveraddr.ipstr, servers[i].serveraddr.port, 1, 1);
		if (servers[i].fd != -1)
		{
			epoll_add(epfd, servers[i].fd, EPOLLIN, &servers[i]);
			repeater_server_description_init(&servers[i]);
			log_d("listen success: %s\n", servers[i].description);
		}
		else
		{
			log_d("listen err, %s, %s\n", servers[i].description, strerror(errno));
		}
	}

	// struct repeater_server repserver;
	// {
	// 	//192.168.0.198 admin Jzlx20150714!
	// 	//repeater_server_init(&repserver, "0.0.0.0", 8082, "14.215.177.38", 80);
	// 	//repeater_server_init(&repserver, "192.168.0.21", 80, "192.168.0.140", 80); HK
	// 	repeater_server_init(&repserver, "192.168.0.21", 80, "192.168.0.198", 80);
	// 	repserver.serveraddr.port_rtsp = 0; //554;
	// 	repserver.toaddr.port_rtsp = 0;		//554;

	// 	repserver.fd = socket_bind_tcp(repserver.serveraddr.ipstr, repserver.serveraddr.port, 1, 1);
	// 	if (repserver.fd == -1)
	// 	{
	// 		perror("not create \n");
	// 	}
	// 	else
	// 	{
	// 		epoll_add(epfd, repserver.fd, EPOLLIN, &repserver);
	// 	}
	// }

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
			repseritem = ((typeof(repseritem))(events[i].data.ptr));

			{
				if (events[i].events & EPOLLIN == EPOLLIN)
				{
					addrlen = sizeof(inaddr);
					fdin = accept(repseritem->fd, (struct sockaddr *)&inaddr, &addrlen);
					if (fdin <= 0)
					{
						continue;
					}

					stream = (struct repeater_stream *)malloc(sizeof(struct repeater_stream));
					memset(stream, 0, sizeof(struct repeater_stream));
					memcpy(&stream->repserver, repseritem, sizeof(*repseritem));
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

					pthread_t pt;
					int ret = pthread_create(&pt, NULL, thread_run, stream);
					if (-1 == ret)
					{
						close(fdin);
						free(stream);
					}
				}
				continue;
			}
		}
	}
	close(epfd);
}

static struct repeater_server _server[512];
static int _server_count = 0;

void TcpAccess_Add2(const char *ethname1, const char *srcip, int srcport, int srcport_rtsp,
					const char *ethname2, const char *dstip, int dstport, int dstport_rtsp)
{
	struct repeater_server *sitem = &_server[_server_count];

	repeater_server_init(sitem, srcip, srcport, srcport_rtsp, dstip, dstport, dstport_rtsp);
	strncpy(sitem->serveraddr.ethName, ethname1, 30 - 1);
	strncpy(sitem->toaddr.ethName, ethname2, 30 - 1);

	repeater_server_description_init(sitem);
	_server_count++;
}

void TcpAccess_Add(const char *ethname1, const char *srcip, int srcport,
				   const char *ethname2, const char *dstip, int dstport)
{
	TcpAccess_Add2(ethname1, srcip, srcport, 0, ethname2, dstip, dstport, 0);
}

void TcpAccess_loop()
{
	repeater_dispatcher(_server, _server_count);
}

#define CONFIG_HAVE_TcpAccess_main 1

#if CONFIG_HAVE_TcpAccess_main
int main(int argc, const char **argv)
{
	//signal(SIGINT, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	// char *src = "GET /mp4/images/ HTTP/1.1\r\n"
	// 			"Host: 127.0.0.1:8082\r\n"
	// 			"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:65.0) Gecko/20100101 Firefox/65.0"
	// 			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
	// 			"Accept-Language: zh-CN,en-US;q=0.7,en;q=0.3\r\n"
	// 			"Accept-Encoding: gzip, deflate\r\n"
	// 			"Referer: http://127.0.0.1:8082/mp4/images/css/\r\n"
	// 			"Connection: keep-alive\r\n"
	// 			"Cookie: BD_UPN=133352; H_PS_645EC=69b1RTU5Wl2O08cV%2FB9eRy8VjT39d4D7H2YX9veNW3ouiTje5VcxDvfIAG0\r\n"
	// 			"Upgrade-Insecure-Requests: 1\r\n"
	// 			"\r\n0xFFFFF";
	//int len = http_heard_getlength(src, strlen(src));
	//char *ptr = ex_strndup(src, len);
	//
	//printf("%s", ptr);
	//printf(">>> %d >> %ld\n", len, strlen(ptr));
	//
	// char buff[500];
	// int blen = 0;
	// str_replace(src, "127.0.0.1", "9999999999", buff, sizeof(buff), &blen);
	// printf("%s", buff);
	// printf("len=%ld, blen=%d\n", strlen(buff), blen);

	// {
	// 	char *ptr = str_replace_new(src, "0xFFFFF", "9999999999");
	// 	printf("==============================%ld==\n", strlen(ptr));
	// 	printf("%s", ptr);
	// 	printf("==========================\n");
	// 	free(ptr);
	// }
	// int fd = socket_connect("127.0.0.1", 12345);
	// while (1)
	// {
	// 	if (poll_isin(fd))
	// 	{
	// 		printf("[%d] %02X need read data....\n", time(NULL));
	// 	}
	// }
	struct repeater_server server[5];

	repeater_server_init(&server[0], "192.168.0.21", 80, 0, "192.168.0.198", 80, 0);
	repeater_dispatcher(server, 1);
	return 0;
}
#endif