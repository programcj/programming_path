/*
 * main.c
 *
 *  Created on: 2020年10月30日
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
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

#include "base64.h"
#include "md5.h"
#include "sps_decode.h"

#define DEBUG 1

uint64_t os_time_ms()
{
	struct timespec times =
				{ 0, 0 };
	uint64_t time;
	clock_gettime(CLOCK_MONOTONIC, &times);
	time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	return time;
}

struct databuff
{
	char *data;
	int size;
	int wpos; //
	int rpos; //
};

void databuff_clear(struct databuff *buff)
{
	buff->wpos = 0;
	buff->rpos = 0;
}

int databuff_write_recvfd(struct databuff *buff, int fd, int size)
{
	int ret;
	if (buff->wpos + size > buff->size)
		return -1;
	ret = recv(fd, buff->data + buff->wpos, size, 0);
	if (ret > 0)
		buff->wpos += ret;
	return ret;
}

int databuff_write(struct databuff *buff, void *data, int size)
{
	if (buff->wpos + size > buff->size)
		return -1;
	memcpy(buff->data + buff->wpos, data, size);
	buff->wpos += size;
	return size;
}

struct databuff *databuff_new(int size)
{
	struct databuff *buff = (struct databuff *) malloc(sizeof(struct databuff) + size);
	if (buff == NULL)
		return NULL;
	buff->data = ((char *) buff) + sizeof(struct databuff);
	buff->size = size;
	buff->wpos = 0;
	buff->rpos = 0;
	memset(buff->data, 0, size);
	return buff;
}

void databuff_free(struct databuff *buff)
{
	if (buff)
		free(buff);
}

int string_http_head_get_content_length(const char *buffhead, int headlen)
{
	const char *ptr = buffhead;
	const char *ptr_end = buffhead + headlen;
	int content_length = -1;
	while (ptr < ptr_end)
	{
		if (0 == strncmp(ptr, "Content-Length:", strlen("Content-Length:")))
		{
			content_length = atoi(ptr + strlen("Content-Length:"));
		}
		ptr = strchr(ptr, '\n');
		if (!ptr)
			break;
		if (ptr + 2 < ptr_end)
		{
			if (ptr[1] == '\r' && ptr[2] == '\n')
				break;
		}
		ptr++;
	}
	return content_length;
}

int string_http_heard_getlength(const char *buf, int len)
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
	return 0;
}

struct URLInfo
{
	char prefix[10];
	char host[30];
	int port;
	char useranme[30];
	char password[30];
	const char *path;

	char *Authenticate; //
	char Authorization[300];
};

/***
 v=0
 o=- 2251938210 2251938210 IN IP4 0.0.0.0
 s=Media Server
 c=IN IP4 0.0.0.0
 t=0 0
 a=control:*
 a=packetization-supported:DH
 a=rtppayload-supported:DH
 a=range:npt=now-
 m=video 0 RTP/AVP 96
 a=control:trackID=0
 a=framerate:25.000000
 a=rtpmap:96 H264/90000
 a=fmtp:96 packetization-mode=1;profile-level-id=4D001F;sprop-parameter-sets=J00AH41oBQBbEAA=,KO4EYgA=
 a=recvonly
 m=application 0 RTP/AVP 107
 a=control:trackID=4
 a=rtpmap:107 vnd.onvif.metadata/90000
 a=recvonly
 =======================================================================
 v=0
 o=- 1604995310197096 1 IN IP4 192.168.0.88
 s=profile1
 u=http:///
 e=admin@
 t=0 0
 a=control:*
 a=range:npt=00.000-
 m=video 0 RTP/AVP 96
 b=AS:5000
 a=control:track1
 a=rtpmap:96 H264/90000
 a=recvonly
 a=fmtp:96 profile-level-id=674d00; sprop-parameter-sets=Z00AKpY1QPAET8s3AQEBAg==,aO4xsg==; packetization-mode=1
 m=audio 0 RTP/AVP 8
 b=AS:1000
 a=control:track2
 a=rtpmap:8 pcma/8000
 a=ptime:40
 a=recvonly
 ***/
struct SDPInfo
{
	//m=video 0 RTP/AVP 96
	int media_video_port;
	char media_video_proto[10];
	int media_video_format;

	//a=rtpmap:96 H264/90000
	int media_video_attr_rtpmap_format;
	char media_video_attr_rtpmap_type[10]; //H264
	int media_video_attr_rtpmap_rate;	   //90000

	//a=framerate:25.000000
	int media_video_attr_framerate;

	//a=control:trackID=0
	char media_video_attr_control[20];

	//sps 大小in sprop-parameter-sets
	int width;
	int height;
};

const char *string_readline(const char *str, char *linestr)
{
	const char *ptr = str;
	while (*ptr && *ptr != '\n' && *ptr != '\r')
		ptr++;
	strncpy(linestr, str, ptr - str);
	linestr[ptr - str] = 0;

	if (*ptr == '\r' && *(ptr + 1) == '\n')
		return ptr + 2;
	if (*ptr == '\n')
		return ptr + 1;
	return ptr;
}

//result:size=33
void md5_gtstr(const char *string, char *result)
{
	int i = 0;
	unsigned char digest[16];

	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, string, strlen(string));
	MD5_Final(digest, &context);

	for (i = 0; i < 16; ++i)
	{
		sprintf(&result[i * 2], "%02x", (unsigned int) digest[i]);
	}
	result[32] = 0;
}

void sdp_parse(const char *str, int len, struct SDPInfo *psdpinfo)
{
	//每行读取
	char linestr[200];
	const char *ptr = str;
	int media_desc = 0, media_video = -1;
	while (*ptr)
	{
		ptr = string_readline(ptr, linestr);
		if (strncasecmp(linestr, "m=", 2) == 0)
			media_desc++;
		if (strncasecmp(linestr, "m=video ", strlen("m=video ")) == 0)
		{
			media_video = media_desc;
			sscanf(linestr, "%*[^ ]%d %s %d", &psdpinfo->media_video_port,
						psdpinfo->media_video_proto,
						&psdpinfo->media_video_format);
			continue;
		}
		if (media_desc != media_video)
			continue;
		if (strncasecmp(linestr, "a=framerate:", strlen("a=framerate:")) == 0)
			sscanf(linestr, "%*[^:]:%d", &psdpinfo->media_video_attr_framerate);
		if (strncasecmp(linestr, "a=control:", strlen("a=control:")) == 0)
			sscanf(linestr, "%*[^:]:%s", psdpinfo->media_video_attr_control);

		if (strncasecmp(linestr, "a=rtpmap:", strlen("a=rtpmap:")) == 0)
		{
			sscanf(linestr, "%*[^:]:%d %[^/]/%d", &psdpinfo->media_video_attr_rtpmap_format,
						psdpinfo->media_video_attr_rtpmap_type,
						&psdpinfo->media_video_attr_rtpmap_rate);
		}
		if (strncasecmp(linestr, "a=fmtp:", strlen("a=fmtp:")) == 0)
		{
			//sprop-parameter-sets 取 sps
			char *ptr = linestr;
			char *ptmp = NULL;
			char *sps_base64 = NULL;
			char *sps_b64_1 = NULL;
			char *sps_b64_2 = NULL;
			while (NULL != (ptr = strtok_r(ptr, ";", &ptmp)))
			{
				while (*ptr == ' ')
					ptr++;

				if (strncasecmp(ptr, "sprop-parameter-sets", strlen("sprop-parameter-sets")) == 0)
				{
					sps_base64 = ptr + strlen("sprop-parameter-sets");
					while (*sps_base64 == ' ' || *sps_base64 == '=')
						sps_base64++;
					sps_b64_1 = sps_base64;
					sps_b64_2 = strchr(sps_b64_1, ',');
					if (sps_b64_2 && *sps_b64_2 == ',')
					{
						sps_b64_2++;
					}
					unsigned char out[BASE64_DECODE_OUT_SIZE(strlen(sps_b64_1))];
					//解码
					int outlen = base64_decode(sps_b64_1, strlen(sps_b64_1), out);
					int width = 0, height = 0, fps = 0;
					h264_decode_sps(out, outlen, &width, &height, &fps);
					//printf("[%dx%d]\n", width, height);
					psdpinfo->width = width;
					psdpinfo->height = height;
				}
				ptr = NULL;
			}
		}
		//printf("sdp parline:%s\n", linestr);
	}

	printf("m=video [%d] [%s] [%d]\n", psdpinfo->media_video_port,
				psdpinfo->media_video_proto,
				psdpinfo->media_video_format);
	printf("fps=%d, control:%s\n", psdpinfo->media_video_attr_framerate, psdpinfo->media_video_attr_control);
	printf("rtpmap:%d %s/%d\n", psdpinfo->media_video_attr_rtpmap_format,
				psdpinfo->media_video_attr_rtpmap_type,
				psdpinfo->media_video_attr_rtpmap_rate);
	printf("wh: %dx%d\n", psdpinfo->width, psdpinfo->height);
}

typedef struct _RTSPClient
{
	char *url;
	char *baseURL;
	int fd;
	struct URLInfo urlinfo;
	struct SDPInfo sdpinfo;
	char UserAgent[30];
	int CSeq;
	//unsigned long sessionid;
	char sessionid_str[30];

	struct databuff *buff_rtsp;
	struct databuff *buff_rtcp_rtp;
	void (*callbk_rtp)(struct _RTSPClient *client, int channel, int len, uint8_t *data);

	int width;
	int height;

	uint64_t time_open_start;  //开始打开时间 ms
	uint64_t time_iframe_first; //I帧首次出现时间 ms

} RTSPClient;

int urldecode(const char *url, struct URLInfo *urlinfo)
{
	const char *ptr = url, *pos;
	const char *pend;
	int len = strlen(url);
	int flagauth = 0;

	ptr = strstr(url, "://");
	if (ptr == NULL)
		return -1;
	ptr += 3;
	strncpy(urlinfo->prefix, url, ptr - url);

	pend = strchr(ptr, '/');
	if (pend == NULL)
		pend = url + len;
	//寻找  username:password@host:port

	const char *pname_end = strchr(ptr, ':');
	const char *phost_begin;

	for (phost_begin = pend; phost_begin > ptr; phost_begin--)
	{
		if (*phost_begin == '@')
			break;
	}

	if (*phost_begin == '@' && pname_end < pend)
	{
		flagauth = 1;
		strncpy(urlinfo->useranme, ptr, pname_end - ptr);
		ptr = pname_end + 1;
		strncpy(urlinfo->password, ptr, phost_begin - ptr);
		ptr = phost_begin + 1;
	}
	//
	if (ptr >= pend)
	{
		printf("[%s]unknow host port\n", url);
		return -1;
	}
	//host and port;
	for (pos = ptr; pos < pend; pos++)
		if (*pos == ':')
			break;
	strncpy(urlinfo->host, ptr, pos - ptr);
	if (*pos == ':')
		ptr = pos + 1;
	else
		ptr = pos;

	if (ptr < pend)
	{
		urlinfo->port = atoi(ptr);
		ptr = pend;
	}
	else
	{
		if (strcmp(urlinfo->prefix, "http://") == 0)
			urlinfo->port = 80;
		if (strcmp(urlinfo->prefix, "https://") == 0)
			urlinfo->port = 443;
	}
	urlinfo->path = ptr;

#if DEBUG
	printf("prefix:[%s]\n", urlinfo->prefix);
	printf("useranme:[%s]\n", urlinfo->useranme);
	printf("password:[%s]\n", urlinfo->password);
	printf("host:[%s]\n", urlinfo->host);
	printf("port:[%d]\n", urlinfo->port);
	printf("path %s\n", urlinfo->path);
#endif
	return 0;
}

const char *urlpathptr(const char *url)
{
	//rtsp://192.168.0.12:8880/E:/mp4/hg_base.h264/
	// /E:/mp4/hg_base.h264/
	if (url == NULL)
		return NULL;
	const char *ptr = strstr(url, "://");
	if (NULL == ptr)
	{
		ptr = strstr(url, "/");
		return ptr;
	}
	ptr += 3;

	const char *pspilt = strchr(ptr, '/');
	if (pspilt)
		return pspilt;
	return NULL;
}

int socket_tcp(const char *host, int port)
{
	int ret;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in in;

	memset(&in, 0, sizeof(in));
	in.sin_family = AF_INET;
	in.sin_port = htons(port);
	ret = inet_pton(AF_INET, host, &in.sin_addr.s_addr);
	if (ret < 0)
	{
		fprintf(stderr, "host not ipaddress [%s]\n", host);
		goto _err;
	}

	ret = connect(fd, (struct sockaddr *) &in, sizeof(in));
	if (ret != 0)
	{
		fprintf(stderr, "not connect ot host [%s], %s\n", host, strerror(errno));
		goto _err;
	}
	return fd;

	_err:
	if (fd != -1)
		close(fd);
	return -1;
}

int socket_send_text(int fd, const char *buf)
{
	printf(">>>>>>>>>>>>>>\n%s", buf);
	int len = strlen(buf);
	int ret;
	int pos = 0;
	int slen = 0;
	while (pos < len)
	{
		ret = send(fd, buf + pos, len - pos, 0);
		if (ret > 0)
			pos += ret;
		if (ret <= 0)
		{
			if (ret < 0 && (errno == EAGAIN))
				continue;
			return -1;
		}
	}
	return pos;
}

int socket_recv_text(int fd, char *buf, int len)
{
	return recv(fd, buf, len, 0);
}

#if 0
int rtsp_response_recv(int fd, char **text, size_t *textlen)
{
	struct pollfd pofd;
	int ret;

	memset(&pofd, 0, sizeof(pofd));
	pofd.fd = fd;
	pofd.events = POLLIN;

	char *membuf = NULL;
	size_t memlen = 0;

	FILE *fp = open_memstream(&membuf, &memlen);
	char buff[1024];
	int content_length = 0;
	int head_lenght = -1;
	int loop = 1;

	while (loop)
	{
		ret = poll(&pofd, 1, 100);
		if (ret == 0)
		continue;
		//先接收4个长度
		if (ret < 0)
		break;

		if (POLLIN == (pofd.revents & POLLIN))
		{
			/**
			 * 1字节接收,再接收内容
			 */
			if (POLLIN == (pofd.revents & POLLIN))
			{
				ret = recv(fd, buff, 1, 0); //不能一下接收完
				if (ret <= 0)
				{
					if (ret < 0 && (errno == EAGAIN || errno == EINTR))
					{
						printf("非阻塞继续\n");
						continue;
					}
					printf("接收错误\n");
					break;
				}
				if (ret > 0)
				{
					fwrite(buff, 1, ret, fp);
					fflush(fp);
				}

				if (head_lenght == -1)
				{
					head_lenght = string_http_heard_getlength(membuf, memlen);
					if (head_lenght != -1)
					content_length = string_http_head_get_content_length(membuf, head_lenght);
					if (content_length == -1)
					content_length = 0;
				}

				if (head_lenght != -1)
				{
					if (content_length + head_lenght == memlen)
					break;
				}
			}
		}
	}
	buff[0] = 0;
	fwrite(buff, 1, 1, fp);
	fflush(fp);
	fclose(fp);

	if (memlen == 0)
	{
		if (membuf != NULL)
		{
			free(membuf);
			membuf = NULL;
		}
	}

#if DEBUG
	printf("================\n%s\n=============\n", membuf);
#endif

	if (memlen > 0)
	{
		char pref[10];
		int code = -1;
		if (2 == sscanf(membuf, "%[^ ] %d ", pref, &code))
		{
			*text = membuf;
			*textlen = memlen;
			return code;
		}
		else
		{
			if (membuf != NULL)
			free(membuf);
		}
	}
	return -1;
}
#endif

int RTSPClient_RTSPRecv(RTSPClient *client);

int rtsp_OPTIONS(RTSPClient *client)
{
	int ret;
	char buff[1024];
	char *ptr = buff;
	struct URLInfo *urlinfo = &client->urlinfo;
	ptr += sprintf(buff, "OPTIONS %s%s:%d%s RTSP/1.0\r\n", urlinfo->prefix, urlinfo->host, urlinfo->port, urlinfo->path);
	ptr += sprintf(ptr, "CSeq: %d\r\n", client->CSeq);
	ptr += sprintf(ptr, "User-Agent: %s\r\n", client->UserAgent);
	ptr += sprintf(ptr, "\r\n");
	ret = socket_send_text(client->fd, buff);
	if (ret < 0)
		return -1;
	client->CSeq++;

	ret = RTSPClient_RTSPRecv(client);
	if (ret < 0)
		return -1;

	char pref[10];
	int code = 0;
	sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);
	return code;
}

int rtsp_DESCRIBE_base(RTSPClient *client)
{
	int ret;
	char buff[1024];
	char *ptr = buff;
	int fd = client->fd;
	struct URLInfo *urlinfo = &client->urlinfo;
	struct SDPInfo *psdpinfo = &client->sdpinfo;

	ptr += sprintf(buff, "DESCRIBE %s%s:%d%s RTSP/1.0\r\n", urlinfo->prefix, urlinfo->host, urlinfo->port, urlinfo->path);
	ptr += sprintf(ptr, "CSeq: %d\r\n", client->CSeq);
	if (strlen(urlinfo->Authorization) > 0)
		ptr += sprintf(ptr, "%s", urlinfo->Authorization);
	ptr += sprintf(ptr, "User-Agent: %s\r\n", client->UserAgent);
	ptr += sprintf(ptr, "Accept: application/sdp\r\n");
	ptr += sprintf(ptr, "\r\n");
	ret = socket_send_text(fd, buff);
	if (ret < 0)
		return -1;
	client->CSeq++;

	ret = RTSPClient_RTSPRecv(client);
	if (ret < 0)
		return -1;

	char pref[10];
	int code = 0;
	sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);

	if (code == 401)
	{
		//需要加密发送哦
		const char *auth = strstr(client->buff_rtsp->data, "WWW-Authenticate:");
		if (auth)
		{
			auth += strlen("WWW-Authenticate:");
			if (*auth == ' ')
				auth++;
			char *next = strchr(auth, '\r');
			if (next == NULL)
				next = strchr(auth, '\n');
			if (next)
				*next = 0;

			char *Authenticate = (char *) calloc(next - auth + 1, 1);
			strcpy(Authenticate, auth);
			if (urlinfo->Authenticate)
				free(urlinfo->Authenticate);
			urlinfo->Authenticate = Authenticate;
		}
	}

	if (code == 200)
	{
		//需要得到 sdp内容
		int head_len = string_http_heard_getlength(client->buff_rtsp->data, client->buff_rtsp->wpos);
		int size = client->buff_rtsp->wpos;

		const char *pbaseUrl = strstr(client->buff_rtsp->data, "Content-Base:");
		const char *end = NULL;
		if (pbaseUrl)
		{
			pbaseUrl += strlen("Content-Base:");
			while (*pbaseUrl == ' ')
				pbaseUrl++;
			end = pbaseUrl;
			while (*end && *end != '\n' && *end != '\r')
				end++;
			if (end - pbaseUrl > 0)
			{
				int baseURL_len = end - pbaseUrl + 1;
				char *baseUrl = (char *) malloc(baseURL_len);
				baseUrl[baseURL_len - 1] = 0;
				strncpy(baseUrl, pbaseUrl, baseURL_len - 1);
				printf("baseURL:%s\n", baseUrl);
				client->baseURL = baseUrl;
			}
		}
//		else
//		{
//			int baseURL_len = strlen(client->urlinfo.path) + 2;
//			char *baseUrl = (char *) calloc(baseURL_len, 1);
//			strncpy(baseUrl, client->urlinfo.path, baseURL_len - 2);
//			int len=strlen(baseUrl);
//			if(len==0 || baseUrl[len-1]!='/')
//				strcat(baseUrl, "/");
//			client->baseURL = baseUrl;
//		}
		printf("baseURL:%s\n", client->baseURL);
		printf("--------SDP数据----------\n%s", client->buff_rtsp->data + head_len);
		//m: media descriptions
		//a: attributes
		sdp_parse(client->buff_rtsp->data + head_len, size - head_len, psdpinfo);
	}
	return code;
}

void printstr_size(const char *ptr, int len)
{
	while (len)
	{
		putchar(*ptr++);
		len--;
	}
	printf("\n");
}

//realm：表示Web服务器中受保护文档的安全域（比如公司财务信息域和公司员工信息域），用来指示需要哪个域的用户名和密码
//qop：保护质量，包含auth（默认的）和auth-int（增加了报文完整性检测）两种策略，（可以为空，但是）不推荐为空值
//nonce：服务端向客户端发送质询时附带的一个随机数，这个数会经常发生变化。客户端计算密码摘要时将其附加上去，使得多次生成同一用户的密码摘要各不相同，用来防止重放攻击
//nc：nonce计数器，是一个16进制的数值，表示同一nonce下客户端发送出请求的数量。例如，在响应的第一个请求中，客户端将发送“nc=00000001”。这个指示值的目的是让服务器保持这个计数器的一个副本，以便检测重复的请求
//cnonce：客户端随机数，这是一个不透明的字符串值，由客户端提供，并且客户端和服务器都会使用，以避免用明文文本。这使得双方都可以查验对方的身份，并对消息的完整性提供一些保护
//response：这是由用户代理软件计算出的一个字符串，以证明用户知道口令
//Authorization-Info：用于返回一些与授权会话相关的附加信息
//nextnonce：下一个服务端随机数，使客户端可以预先发送正确的摘要
//rspauth：响应摘要，用于客户端对服务端进行认证
//stale：当密码摘要使用的随机数过期时，服务器可以返回一个附带有新随机数的401响应，并指定stale=true，表示服务器在告知客户端用新的随机数来重试，而不再要求用户重新输入用户名和密码了
void digest_calc(struct URLInfo *urlinfo, const char *requestcmd, const char *uri)
{
	//WWW-Authenticate: Digest realm="RTSP SERVER", nonce="8f41543422d38188e383815983a30377", stale="FALSE"
	//Authorization: Digest username="admin", realm="RTSP SERVER", nonce="8f41543422d38188e383815983a30377", uri="rtsp://192.168.0.88:554/profile1", response="519bd2f50627f890ca0e6c8c1a69a95a"
	printf("digest: [%s]\n", urlinfo->Authenticate);
	const char *ptr = urlinfo->Authenticate;
	int authlen = strlen(urlinfo->Authenticate);
	const char *pend = ptr + authlen;
	const char *psplit = NULL;

	ptr = strstr(ptr, "Digest");
	if (ptr)
		ptr += strlen("Digest") + 1;

	const char *pname = NULL;
	const char *pvalue = NULL;

	char realm_str[100];
	char nonce_str[33];
	char stale_str[10];

	memset(realm_str, 0, sizeof(realm_str));
	memset(nonce_str, 0, sizeof(nonce_str));
	memset(stale_str, 0, sizeof(stale_str));

	while (*ptr)
	{
		if (pname == NULL)
			pname = ptr;

		if (pname && *ptr == '=')
		{
			int name_len = ptr - pname;
			ptr++;
			while (*ptr && *ptr != '\r' && *ptr != '\n' && *ptr == ' ')
				ptr++;
			pvalue = ptr;

			int pvalue_len = 0;
			if (*pvalue == '\"')
			{
				ptr++;
				pvalue++;
				while (*ptr && *ptr != '\r' && *ptr != '\n' && *ptr != '\"') //寻找下一个"
					ptr++;
				pvalue_len = ptr - pvalue;
			}
			//, 或 \r\n 或 \0结尾
			while (*ptr && *ptr != '\r' && *ptr != '\n' && *ptr != ',')
				ptr++;
			//value
			if (pvalue_len == 0)
				pvalue_len = ptr - pvalue;
			while (*ptr && *ptr != '\r' && *ptr != '\n' && (*ptr == ',' || *ptr == ' '))
				ptr++;

			printf("name=");
			printstr_size(pname, name_len);
			printf("value=");
			printstr_size(pvalue, pvalue_len);

			if (strncasecmp(pname, "realm=", strlen("realm=")) == 0)
				strncpy(realm_str, pvalue, pvalue_len);
			if (strncasecmp(pname, "nonce=", strlen("nonce=")) == 0)
				strncpy(nonce_str, pvalue, pvalue_len);
			if (strncasecmp(pname, "stale=", strlen("stale=")) == 0)
				strncpy(stale_str, pvalue, pvalue_len);

			pvalue = NULL;
			pname = NULL;
			continue;
		}
		ptr++;
	}
	//需要进行md5加密
	//	算法	A1
	//	MD5（默认）	<username>:<realm>:<password>
	//	MD5-sess(algorithm=“md5-sess”)	MD5(<username>:<realm>:<password>):<nonce>:<cnonce>
	//
	char A1Str[300];
	char A1Md5_def[33];

	sprintf(A1Str, "%s:%s:%s", urlinfo->useranme, realm_str, urlinfo->password);
	md5_gtstr(A1Str, A1Md5_def);
	printf("A1:%s\n", A1Str);
	printf("A1Md5_def:%s\n", A1Md5_def);

	char *nc = "";
	char *cnonce = "";
	char *qop = "";

	//如果qop有值(if (*pszQop))，则	HD=nonce:noncecount:cnonce:qop
	//否则HD=nonce

	/// The “response” field is computed as:
	// md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
	// or, if “fPasswordIsMD5” is True:
	// md5(<password>:<nonce>:md5(<cmd>:<url>))

	char A2Str[300];
	char A2md5[33];							  // "rtsp://192.168.0.88:554/profile1"
	sprintf(A2Str, "%s:%s", requestcmd, uri); //DESCRIBE
	md5_gtstr(A2Str, A2md5);

	char md5src[300];
	sprintf(md5src, "%s:%s:%s", A1Md5_def, nonce_str, A2md5);

	char response[33];
	md5_gtstr(md5src, response);
	printf("A2Str=%s, md5str=%s\n", A2Str, response);

	sprintf(urlinfo->Authorization, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
				urlinfo->useranme, realm_str, nonce_str, uri,
				response);
}

int rtsp_DESCRIBE(RTSPClient *client)
{
	int ret;
	ret = rtsp_DESCRIBE_base(client);
	if (ret == 401 && client->urlinfo.Authenticate)
	{
		if (strncmp(client->urlinfo.Authenticate, "Basic ", 6) == 0)
		{
			char pwd[100];
			char base64str[200];
			snprintf(pwd, sizeof(pwd), "%s:%s", client->urlinfo.useranme, client->urlinfo.password);
			base64_encode(pwd, strlen(pwd), base64str);
			sprintf(client->urlinfo.Authorization, "Authorization: Basic %s\r\n", base64str);
		}
		if (strncasecmp(client->urlinfo.Authenticate, "Digest ", strlen("Digest ")) == 0)
		{
			char uri[200];
			sprintf(uri, "%s%s:%d%s", client->urlinfo.prefix,
						client->urlinfo.host,
						client->urlinfo.port,
						client->urlinfo.path);
			digest_calc(&client->urlinfo, "DESCRIBE", uri);
		}
		ret = rtsp_DESCRIBE_base(client);
		if (ret == 401)
			return 401;
	}
	return ret;
}

//客户端提醒服务器建立会话,并确定传输模式:
/**
 ================
 SETUP rtsp://192.168.0.150:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif/trackID=0 RTSP/1.0
 Transport: RTP/AVP/TCP;unicast;interleaved=0-1
 x-Dynamic-Rate: 0
 CSeq: 3
 User-Agent: CJRTSPClientV1.0
 Authorization: Basic YWRtaW46YWRtaW4=
 ================
 RTSP/1.0 200 OK
 CSeq: 3
 Session: 309908140000;timeout=60
 Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=44BB06DC
 x-Dynamic-Rate: 1
 **/
int rtsp_SETUP(RTSPClient *client)
{
	char headstr[1024];
	char *ptr = headstr;
	struct URLInfo *urlinfo = &client->urlinfo;
	int ret;
	char url[300];

	const char *urlbase_path = urlpathptr(client->baseURL);
	if (urlbase_path == NULL)
		urlbase_path = urlinfo->path;
	char *pathsplit = "";
	if (strlen(urlbase_path) > 0 && (urlbase_path[strlen(urlbase_path) - 1] != '/'))
	{
		//需要增加/
		pathsplit = "/";
	}
	snprintf(url, sizeof(url), "%s%s:%d%s%s%s", urlinfo->prefix, urlinfo->host,
				urlinfo->port,
				urlbase_path,
				pathsplit,
				client->sdpinfo.media_video_attr_control);

	ptr += sprintf(ptr, "SETUP %s RTSP/1.0\r\n", url);
	ptr += sprintf(ptr, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
	//ptr += sprintf(ptr, "Transport: RTP/AVP;unicast;client_port=57446-57447\r\n"); //UDP
	ptr += sprintf(ptr, "x-Dynamic-Rate: 0\r\n");
	ptr += sprintf(ptr, "CSeq: %d\r\n", client->CSeq);
	ptr += sprintf(ptr, "User-Agent: %s\r\n", client->UserAgent);
	if (strlen(client->sessionid_str) > 0)
		ptr += sprintf(ptr, "Session: %s\r\n", client->sessionid_str);

	if (strlen(urlinfo->Authorization) > 0)
	{
		if (strncasecmp(client->urlinfo.Authenticate, "Digest ", strlen("Digest ")) == 0)
		{
			digest_calc(&client->urlinfo, "SETUP", url);
		}
		ptr += sprintf(ptr, "%s", urlinfo->Authorization);
	}
	ptr += sprintf(ptr, "\r\n");

	ret = socket_send_text(client->fd, headstr);
	if (ret < 0)
		return -1;

	ret = RTSPClient_RTSPRecv(client);
	if (ret < 0)
		return -1;
	client->CSeq++;
	char pref[10];
	int code = 0;
	sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);

	if (code == 200)
	{
		//Session: 194909530000;timeout=60
		char *sessionptr = strstr(client->buff_rtsp->data, "Session:");
		if (sessionptr)
		{
			sessionptr += strlen("Session:") + 1;
			const char *pend = strchr(sessionptr, ';');
			if (pend == NULL)
				pend = strchr(sessionptr, '\r');
			if (pend == NULL)
				pend = strchr(sessionptr, '\n');
			while (*sessionptr == ' ')
				sessionptr++;
			int len = pend - sessionptr;
			if (len > 0)
			{
				strncpy(client->sessionid_str, sessionptr, len);
			}
			printf("Sessionid:[%s]", client->sessionid_str);
		}
		return 200;
	}
	return code;
}

int rtsp_PLAY(RTSPClient *client)
{
	char headstr[1024];
	char *ptr = headstr;
	struct URLInfo *urlinfo = &client->urlinfo;
	int ret;

	const char *urlbase_path = urlpathptr(client->baseURL);
	if (urlbase_path == NULL)
		urlbase_path = urlinfo->path;

	ptr += sprintf(ptr, "PLAY %s%s:%d%s RTSP/1.0\r\n",
				urlinfo->prefix, urlinfo->host,
				urlinfo->port,
				urlbase_path);

	//信道0和1对流数据以及控制信息进行交织
	ptr += sprintf(ptr, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
	//ptr += sprintf(ptr, "Transport: RTP/AVP;unicast;client_port=57446-57447\r\n"); //UDP
	ptr += sprintf(ptr, "CSeq: %d\r\n", client->CSeq);
	ptr += sprintf(ptr, "User-Agent: %s\r\n", client->UserAgent);
	ptr += sprintf(ptr, "Session: %s\r\n", client->sessionid_str);
	ptr += sprintf(ptr, "Range: npt=0.000-\r\n");

	if (strlen(urlinfo->Authorization) > 0)
	{
		if (strncasecmp(client->urlinfo.Authenticate, "Digest ", strlen("Digest ")) == 0)
		{
			char uri[200];
			sprintf(uri, "%s%s:%d%s/", client->urlinfo.prefix,
						client->urlinfo.host,
						client->urlinfo.port,
						client->urlinfo.path);
			digest_calc(&client->urlinfo, "PLAY", uri);
		}
		ptr += sprintf(ptr, "%s", urlinfo->Authorization);
	}
	ptr += sprintf(ptr, "\r\n");

	ret = socket_send_text(client->fd, headstr);
	if (ret < 0)
		return -1;

	ret = RTSPClient_RTSPRecv(client);
	if (ret < 0)
		return -1;
	client->CSeq++;
	char pref[10];
	int code = 0;
	sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);

	if (code == 200)
		return 200;
	return code;
}

int rtsp_GET_PARAMETER(RTSPClient *client)
{
	char headstr[1024];
	char *ptr = headstr;
	struct URLInfo *urlinfo = &client->urlinfo;
	int ret;

	const char *urlbase_path = urlpathptr(client->baseURL);
	if (urlbase_path == NULL)
		urlbase_path = urlinfo->path;

	ptr += sprintf(ptr, "GET_PARAMETER %s%s:%d%s RTSP/1.0\r\n",
				urlinfo->prefix, urlinfo->host,
				urlinfo->port,
				urlbase_path);

	ptr += sprintf(ptr, "CSeq: %d\r\n", client->CSeq);
	ptr += sprintf(ptr, "User-Agent: %s\r\n", client->UserAgent);
	ptr += sprintf(ptr, "Session: %s\r\n", client->sessionid_str);
	if (strlen(urlinfo->Authorization) > 0)
	{
		if (strncasecmp(client->urlinfo.Authenticate, "Digest ", strlen("Digest ")) == 0)
		{
			char uri[200];
			sprintf(uri, "%s%s:%d%s/", client->urlinfo.prefix,
						client->urlinfo.host,
						client->urlinfo.port,
						client->urlinfo.path);
			digest_calc(&client->urlinfo, "PLAY", uri);
		}
		ptr += sprintf(ptr, "%s", urlinfo->Authorization);
	}
	ptr += sprintf(ptr, "\r\n");
	ret = socket_send_text(client->fd, headstr);
	if (ret < 0)
		return -1;
	return 0;
}
void RTSPClient_close(RTSPClient *client);

RTSPClient *RTSPClient_open(const char *url)
{
	RTSPClient *client = (RTSPClient *) calloc(sizeof(RTSPClient), 1);
	if (client == NULL)
		return NULL;
	client->buff_rtsp = databuff_new(2048);
	client->buff_rtcp_rtp = databuff_new(1024 * 200); //max 65535
	client->time_open_start = os_time_ms();
	strcpy(client->UserAgent, "CJRTSPClientV1.0");
	client->url = strdup(url);
	urldecode(client->url, &client->urlinfo);
	int ret;
	int fd = socket_tcp(client->urlinfo.host, client->urlinfo.port);
	if (fd == -1)
	{
		RTSPClient_close(client);
		return NULL;
	}
	printf("connect success %d,[%s]\n", fd, client->urlinfo.host);
	client->fd = fd;
	ret = rtsp_OPTIONS(client);
	ret = rtsp_DESCRIBE(client);
	if (ret != 200)
		goto _err;
	ret = rtsp_SETUP(client);
	if (ret != 200)
		goto _err;
	ret = rtsp_PLAY(client);
	if (ret != 200)
		goto _err;
	return client;

	_err:
	RTSPClient_close(client);
	return NULL;
}

void RTSPClient_close(RTSPClient *client)
{
	if (client->fd != -1 && client->fd != 0)
		close(client->fd);
	if (client->urlinfo.Authenticate)
		free(client->urlinfo.Authenticate);
	if (client->url)
		free(client->url);
	if (client->buff_rtsp)
		databuff_free(client->buff_rtsp);
	if (client->buff_rtcp_rtp)
		databuff_free(client->buff_rtcp_rtp);
	free(client);
}

/**
 * 5.1 RTP Fixed Header Fields
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           timestamp                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |           synchronization source (SSRC) identifier            |
 *  +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *  |            contributing source (CSRC) identifiers             |
 *  |                             ....                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  version (V): 2 bits
 *  padding (P): 1 bit
 *  extension (X): 1 bit
 *  CSRC count (CC): 4 bits
 *  marker (M): 1 bit
 *  payload type (PT): 7 bits
 *  sequence number: 16 bits
 *  timestamp: 32 bits
 *  SSRC: 32 bits
 *  CSRC list: 0 to 15 items, 32 bits each
 */
struct rtp_pkt_header
{
	union
	{
		uint16_t flags;
		struct info
		{
#if __BYTE_ORDER == __LITTLE_ENDIAN
			uint8_t cc :4;
			uint8_t x :1; //ex
			uint8_t p :1;
			uint8_t v :2;

			///
			uint8_t PT :7;
			uint8_t M :1;
#endif
		} info;
	} _base;
	uint16_t seqnum;
	uint32_t timestamp;
	uint32_t ssrc;
}__attribute__((packed));

/*
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |F|NRI|  Type   |
 +---------------+
 */
typedef struct
{
	//byte 0
	unsigned char TYPE :5;
	unsigned char NRI :2;
	unsigned char F :1;
} NALU_HEADER; // 1 BYTE

/*
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |F|NRI|  Type   |
 +---------------+
 */
typedef struct
{
	//byte 0
	unsigned char TYPE :5;
	unsigned char NRI :2;
	unsigned char F :1;
} FU_INDICATOR; // 1 BYTE

/*
 +---------------+
 |0|1|2|3|4|5|6|7|
 +-+-+-+-+-+-+-+-+
 |S|E|R|  Type   |
 +---------------+
 */
typedef struct
{
	//byte 0
	unsigned char TYPE :5;
	unsigned char R :1;
	unsigned char E :1;
	unsigned char S :1;
} FU_HEADER; // 1 BYTES

//H264定义的类型 values for nal_unit_type
enum
{
	NALU_TYPE_SLICE = 1,
	NALU_TYPE_DPA = 2,
	NALU_TYPE_DPB = 3,
	NALU_TYPE_DPC = 4,
	NALU_TYPE_IDR = 5,
	NALU_TYPE_SEI = 6,
	NALU_TYPE_SPS = 7,
	NALU_TYPE_PPS = 8,
	NALU_TYPE_AUD = 9,
	NALU_TYPE_EOSEQ = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL = 12
};

int _http_recv(int fd, struct databuff *buff, int *http_head_length, int *http_content_length)
{
	int ret;
	char _packet[1];

	ret = recv(fd, _packet, 1, 0); //不能一下接收完(因为不知道是否有非http数据)
	//ret = databuff_write_recvfd(buff, fd, 1);
	if (ret <= 0)
	{
		if (ret < 0 && (errno == EAGAIN || errno == EINTR))
			return 0;
		fprintf(stderr, "接收错误 close,[%s]\n", strerror(errno));
		return -1;
	}

	ret = databuff_write(buff, _packet, 1);
	if (ret < 0)
	{
		fprintf(stderr, "超过接收缓存大小:%d\n", buff->size);
		return ret;
	}

	if (*http_head_length == 0)
	{
		*http_head_length = string_http_heard_getlength(buff->data, buff->wpos);
		if (*http_head_length != 0)
			*http_content_length = string_http_head_get_content_length(buff->data, *http_head_length);
		if (*http_content_length == -1)
			*http_content_length = 0; //没有内容
	}
	return ret;
}

int _RTSPClient_RecvRTSPTextMsg(RTSPClient *client)
{
	struct pollfd pofd;
	int ret;
	int fd = client->fd;
	int recv_ret;
	char _packet[1];
	int content_length = 0;
	int head_lenght = 0;

	while (1)
	{
		memset(&pofd, 0, sizeof(pofd));
		pofd.fd = client->fd;
		pofd.events = POLLIN;
		ret = poll(&pofd, 1, 100);
		if (ret == 0)
			continue;
		if (ret < 0)
			break;
		if (POLLIN != (pofd.revents & POLLIN))
			continue;
		ret = _http_recv(fd, client->buff_rtsp, &head_lenght, &content_length);
		if (ret == -1)
			return -1;

		if (head_lenght != 0 && content_length + head_lenght == client->buff_rtsp->wpos)
		{
			client->buff_rtsp->data[client->buff_rtsp->wpos] = 0;
			printf("<<<<<<<<<\n%s", client->buff_rtsp->data);
			char pref[10];
			int code = -1;
			ret = sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);
			return code;
		}
	}
	return -1;
}

int RTSPClient_RTSPRecv(RTSPClient *client)
{
	int loop = 1;
	struct pollfd pofd;
	int ret;
	int fd = client->fd;

	char _packet_head[4];
	int rflag = 0;
	int recv_ret = 0;
	int pos = 0;

	uint8_t RTSPIF_Channel = 0; //1Byte
	uint16_t RTSPIF_len = 0;	//65535 2Byte

	int content_length = 0;
	int head_lenght = 0;

	while (loop)
	{
		memset(&pofd, 0, sizeof(pofd));
		pofd.fd = client->fd;
		pofd.events = POLLIN;
		ret = poll(&pofd, 1, 100);
		if (ret == 0)
			continue;
		if (ret < 0)
			break;
		if (POLLIN != (pofd.revents & POLLIN))
			continue;

		if (rflag == 0)
		{
			recv_ret = recv(fd, _packet_head + pos, 1, 0); //不能一下接收完
			if (recv_ret <= 0)
			{
				if (recv_ret < 0 && (errno == EAGAIN || errno == EINTR))
					continue;
				printf("接收错误\n");
				return -1;
				break;
			}
			pos++;
			if (pos == 4)
			{
				pos = 0;
				if (memcmp(_packet_head, "RTSP", 4) == 0)
				{ //RTSP resp message
					rflag = 1;
					databuff_clear(client->buff_rtsp);
					databuff_write(client->buff_rtsp, _packet_head, 4);
					continue;
				}
				else if (_packet_head[0] == '$')
				{ //RTSPIF
					rflag = 2;
					RTSPIF_Channel = _packet_head[1];
					memcpy(&RTSPIF_len, _packet_head + 2, 2);
					RTSPIF_len = ntohs(RTSPIF_len);
					//还需要接收RTSPIF_len个数据长度
					//RTCP or RTP
					printf("RTSP Interleaved Frame, magic:%c channel:%d len:%d\n",
								_packet_head[0],
								RTSPIF_Channel,
								RTSPIF_len);

					databuff_clear(client->buff_rtcp_rtp);
					if (RTSPIF_len > client->buff_rtcp_rtp->size)
					{
						fprintf(stderr, "RTP Data len(%d) > buff size(%d)\n", RTSPIF_len, client->buff_rtcp_rtp->size);
						return -1;
					}
					continue;
				}
				else
				{
					printf("未知内容\n");
					loop = 0;
					break;
				}

			} // pos==4
		}
		///
		if (rflag == 1)
		{
			ret = _http_recv(fd, client->buff_rtsp, &head_lenght, &content_length);
			if (ret == -1)
				return -1;

			if (head_lenght != 0 && content_length + head_lenght == client->buff_rtsp->wpos)
			{
				client->buff_rtsp->data[client->buff_rtsp->wpos] = 0;
				printf("<<<<<<<<<\n%s", client->buff_rtsp->data);
				char pref[10];
				int code = -1;
				ret = sscanf(client->buff_rtsp->data, "%[^ ] %d ", pref, &code);
				return code;
			}
		}
		//
		if (rflag == 2)
		{ //RTSPIF 内容
			recv_ret = recv(fd, client->buff_rtcp_rtp->data + client->buff_rtcp_rtp->wpos,
						RTSPIF_len - client->buff_rtcp_rtp->wpos, 0); //不能一下接收完
			if (recv_ret <= 0)
			{
				if (recv_ret < 0 && (errno == EAGAIN || errno == EINTR))
				{
					continue;
				}
				printf("接收错误\n");
				return -1;
				break;
			}
			client->buff_rtcp_rtp->wpos += recv_ret;
			if (client->buff_rtcp_rtp->wpos == recv_ret)
			{
				printf("RTSPIF end\n");
				if (client->callbk_rtp)
					client->callbk_rtp(client, RTSPIF_Channel, RTSPIF_len, client->buff_rtcp_rtp->data);
				databuff_clear(client->buff_rtcp_rtp);
				rflag = 0;
			}
		}
		///
	}
	return 0;
}

int fwrite_nal_split(FILE *fp, uint8_t nal_type)
{
	static char nal_4[4] =
				{ 00, 00, 00, 01 };
	static char nal_3[3] =
				{ 00, 00, 01 };

	NALU_HEADER *nalu_hdr = NULL;
	nalu_hdr = (NALU_HEADER *) &nal_type;
	switch (nalu_hdr->TYPE)
	{
		case NALU_TYPE_SEI:
			case NALU_TYPE_IDR:
			if (fp)
				fwrite(nal_3, 3, 1, fp);
		break;
		case NALU_TYPE_SPS:
			case NALU_TYPE_PPS:
			case NALU_TYPE_SLICE:
			default:
			if (fp)
				fwrite(nal_4, 4, 1, fp);
	}
	return 0;
}

int RTSPClient_StreamRecv(RTSPClient *client)
{
	//与SETUP有关
	//RTP和RTCP数据会以$符号＋1个字节的通道编号＋2个字节的数据长度，共4个字节的前缀开始
	//	| magic number | channel number | data length | data  |magic number -
	//	magic number：   RTP数据标识符，"$" 一个字节
	//	channel number： 信道数字 - 1个字节，用来指示信道
	//	data length ：   数据长度 - 2个字节，用来指示插入数据长度
	//	data ：          数据 - ，比如说RTP包，总长度与上面的数据长度相同
	struct pollfd pofds;
	int ret;

	int fd = client->fd;
	uint8_t _packet_type[4];
	int rflag = 0;
	int _typepos = 0;

	int RTSPIF_magic;
	uint8_t RTSPIF_Channel = 0; //1Byte
	uint16_t RTSPIF_len = 0;	//65535 2Byte

	time_t time_old = time(NULL);

	FILE *fp = NULL;
	int loop = 1;

	printf("视频流接收中哦....\n");

#if 0
	int outfd = open("channel", O_WRONLY);
	fp = fdopen(outfd, "wb");
#endif

#if 1
	fp = fopen("test.h264", "wb");
#endif

	while (loop)
	{
		memset(&pofds, 0, sizeof(pofds));
		pofds.fd = client->fd;
		pofds.events = POLLIN;
		ret = poll(&pofds, 1, 100);
		if (ret < 0)
		{
			printf("poll err\n");
			break;
		}

		if (ret == 0)
		{
			printf("timeout \n");
			continue;
		}
		if (POLLIN != (pofds.revents & POLLIN))
			continue;

		if (rflag == 0) //接收头
		{
			ret = recv(fd, _packet_type + _typepos, 1, 0); //不能一下接收完
			if (ret <= 0)
			{
				if (ret < 0 && (errno == EAGAIN || errno == EINTR))
					continue;
				fprintf(stderr, "接收错误\n");
				return -1;
				break;
			}
			_typepos++;
			if (_typepos == 4)
			{
				_typepos = 0;
				if (memcmp(_packet_type, "RTSP", 4) == 0)
				{
					int rtsp_code;
					rflag = 1;
					databuff_clear(client->buff_rtsp);
					databuff_write(client->buff_rtsp, _packet_type, 4);
					rtsp_code = _RTSPClient_RecvRTSPTextMsg(client);
					if (rtsp_code != 200)
					{
						printf("rtsp err\n");
						loop = 0;
						break;
					}
					printf("rtsp response ok\n");
					rflag = 0;
					continue;
				}
				else
				if (_packet_type[0] == '$')
				{
					RTSPIF_magic = _packet_type[0];
					RTSPIF_Channel = _packet_type[1];
					memcpy(&RTSPIF_len, _packet_type + 2, 2);
					RTSPIF_len = ntohs(RTSPIF_len);
					//https://www.jianshu.com/p/334a4198b250
					//channel: 0 Video RTP;
					//		   1 Video RTCP;
					//  	   2 Audio RTP;
					//         3 Audio RTCP;
					printf("RTSP Interleaved Frame, magic:%c channel:%d len:%d\n",
								RTSPIF_magic,
								RTSPIF_Channel,
								RTSPIF_len);
					switch (RTSPIF_Channel)
					{
						case 0x00:
							printf("Video RTP ");
						break;
						case 0x01:
							printf("Video RTCP ");
						break;
						case 0x02:
							printf("Audio RTP ");
						break;
						case 0x03:
							printf("Audio RTCP ");
						break;
						default:
							break;
					}

					databuff_clear(client->buff_rtcp_rtp);
					if (RTSPIF_len > client->buff_rtcp_rtp->size)
					{
						fprintf(stderr, "RTP Data len(%d) > buff size(%d)\n", RTSPIF_len, client->buff_rtcp_rtp->size);
						return -1;
					}
					rflag = 2;
					continue;
				}
				else
				{

				}
			}
		}
		if (rflag == 1) //HTTP
		{
		}
		if (rflag == 2) //RTP msg
		{
			ret = recv(fd, client->buff_rtcp_rtp->data + client->buff_rtcp_rtp->wpos,
						RTSPIF_len - client->buff_rtcp_rtp->wpos, 0); //不能一下接收完
			if (ret <= 0)
			{
				if (ret < 0 && (errno == EAGAIN || errno == EINTR))
				{
					continue;
				}
				printf("接收错误\n");
				return -1;
				break;
			}
			client->buff_rtcp_rtp->wpos += ret;

			if (client->buff_rtcp_rtp->wpos == RTSPIF_len)
			{
				rflag = 0;

				printf("RTSPIF 接收完成\n");
				{
					if (RTSPIF_Channel == 0) //RTP 头
					{
						struct rtp_pkt_header rtp_hdr;
						char *buffptr = client->buff_rtcp_rtp->data;
						int bufflen = RTSPIF_len;

						memcpy(&rtp_hdr, buffptr, sizeof(rtp_hdr));
						buffptr += sizeof(rtp_hdr);
						RTSPIF_len -= sizeof(rtp_hdr);
						printf("RTP V=%d Padding:%d Extension:%d CC:%d ",
									rtp_hdr._base.info.v,
									rtp_hdr._base.info.p,
									rtp_hdr._base.info.x,
									rtp_hdr._base.info.cc);

						printf("PT=%d ", rtp_hdr._base.info.PT);
						printf("seq:%u ", rtp_hdr.seqnum);
						printf("timestamp:%u ", rtp_hdr.timestamp);
						printf("ssrc:%u\n", rtp_hdr.ssrc);
						//exit(0);
						if (rtp_hdr._base.info.PT != 96)
						{
							printf("非H264包\n");
							continue;
						}

						if (rtp_hdr._base.info.x)
						{ //有扩展头
							unsigned short rtp_ex_profile = 0;
							int rtp_ex_length = 0;
							uint32_t header;

							memcpy(&rtp_ex_profile, buffptr, 2);
							buffptr += 2;
							memcpy(&rtp_ex_length, buffptr, 2);
							buffptr += 2;
							RTSPIF_len -= 4;
							rtp_ex_profile = ntohs(rtp_ex_profile);
							rtp_ex_length = ntohs(rtp_ex_length);

							printf("扩展: %04X len:%d ", rtp_ex_profile, rtp_ex_length);
							int ll = 0;
							for (ll = 0; ll < rtp_ex_length; ll++)
							{
								memcpy(&header, buffptr, 4);
								header = ntohl(header);
								printf("header:%08X ", header);
								buffptr += 4;
								RTSPIF_len -= 4;
							}
						}

						//如果UALU数据＜1500（自己设定），使用单一包发送：
						//如果NALU数据＞1500，使用分片发送：（分三种情况设备：第一次RTP，中间的RTP，最后一次RTP）
						uint8_t *h264_packet = buffptr;
						int h264_packet_len = RTSPIF_len;

						NALU_HEADER *nalu_hdr = (NALU_HEADER *) h264_packet;

						if (nalu_hdr->TYPE > 0 && nalu_hdr->TYPE < 24) //单包
						{
							printf("单包:%d\n", nalu_hdr->TYPE);
							switch (nalu_hdr->TYPE)
							{
								case NALU_TYPE_SPS:
									{
									int width;
									int height;
									int fps;
									h264_decode_sps(h264_packet, h264_packet_len, &width, &height, &fps);
									printf("SPS %dx%d %d fps\n", width, height, fps);
									if (client->time_iframe_first == 0)
									{
										client->time_iframe_first = os_time_ms();
										printf("use time: %"PRIu64"s\n", (client->time_iframe_first - client->time_open_start) / 1000);
										//exit(1);
									}
								}
								break;
								case NALU_TYPE_PPS:
									printf("PPS\n");
								break;
							}
							if (fp)
							{
								fwrite_nal_split(fp, *buffptr);
								fwrite(h264_packet, 1, h264_packet_len, fp);
							}
						}
						else if (nalu_hdr->TYPE == 24) //STAP-A 单一时间的组合包
						{
							printf("STAP-A 单一时间的组合包\n");
						}
						else if (nalu_hdr->TYPE == 28) //FU-A分片包，解码顺序和传输顺序相同
						{
							FU_INDICATOR *fu_ind = (FU_INDICATOR *) h264_packet;
							FU_HEADER *fu_hdr = (FU_HEADER *) (h264_packet + 1);
							h264_packet += 2;
							h264_packet_len -= 2;
//							printf("FU_INDICATOR->F     :%d\n", fu_ind->F);
//							printf("FU_INDICATOR->NRI   :%d\n", fu_ind->NRI);
//							printf("FU_INDICATOR->TYPE  :%d\n", fu_ind->TYPE);
//							printf("FU_HEADER->S        :%d\n", fu_hdr->S);
//							printf("FU_HEADER->E        :%d\n", fu_hdr->E);
//							printf("FU_HEADER->R        :%d\n", fu_hdr->R);
//							printf("FU_HEADER->TYPE     :%d\n", fu_hdr->TYPE);

							if (rtp_hdr._base.info.M == 1) //分片包最后一个包
							{
								if (fp)
									fwrite(h264_packet, 1, h264_packet_len, fp); //写NAL数据
							}
							else if (rtp_hdr._base.info.M == 0) //分片包 但不是最后一个包
							{
								if (fu_hdr->S == 1) //分片的第一个包
								{
									unsigned char F;
									unsigned char NRI;
									unsigned char TYPE;
									unsigned char nh;

									F = fu_ind->F << 7;
									NRI = fu_ind->NRI << 5;
									TYPE = fu_hdr->TYPE; //应用的是FU_HEADER的TYPE
									//nh = n->forbidden_bit|n->nal_reference_idc|n->nal_unit_type;  //二进制文件也是按 大字节序存储
									nh = F | NRI | TYPE;

									switch (TYPE)
									{
										case NALU_TYPE_SPS:
											{
											printf("SPS\n");
											if (client->time_iframe_first == 0)
											{
												client->time_iframe_first = os_time_ms();
												printf("use time: %"PRIu64"s\n", (client->time_iframe_first - client->time_open_start) / 1000);
												//exit(1);
											}
										}
										break;
										case NALU_TYPE_PPS:
											printf("PPS\n");
										break;
									}
									printf("当前包为FU-A分片包第一个包: type=%d\n", TYPE); //写NAL
									fwrite_nal_split(fp, nh);
									if (fp)
									{
										//fwrite(nal_3, 3, 1, fp);
										fwrite(&nh, 1, 1, fp);				//写NAL HEADER
										fwrite(h264_packet, 1, h264_packet_len, fp); //写NAL数据
									}
								}
								else //如果不是第一个包
								{
									printf("当前包为FU-A分片包\n");
									if (fp)
										fwrite(h264_packet, h264_packet_len, 1, fp);
								}
							}
						}
					}
				}
				databuff_clear(client->buff_rtcp_rtp);
			}
		}

		if (abs(time(NULL) - time_old) > 10)
		{
			time_old = time(NULL);
			rtsp_GET_PARAMETER(client);
		}
	}
}

void urldecode_test(const char *url)
{
	struct URLInfo uinfo;
	memset(&uinfo, 0, sizeof(uinfo));
	printf("\nURL:%s\n", url);
	urldecode(url, &uinfo);
}

int main(int argc, char **argv)
{
	const char *url = "rtsp://admin:admin@192.168.0.150:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif";
	url = "rtsp://192.168.0.12:8880/hg_base.h264";
	url = "rtsp://admin:admin@192.168.0.88:554/profile1";
	url = "rtsp://192.168.0.27:554/sip/44030500001320190006";
	if (argc > 1)
		url = argv[1];
	printf("url:%s\n", url);
//rtsp://admin:admin@192.168.0.121:554/profile1
//rtsp://admin:admin@192.168.0.161:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif
#if 0
	urldecode_test("rtsp://admin:admin@11@192.168.0.150:554/paht1");
	urldecode_test("rtsp://admin:@192.168.0.150:554/paht1");
	urldecode_test("rtsp://192.168.0.150:554/paht1");
	urldecode_test("rtsp://:554/paht1");
	urldecode_test("rtsp:///paht1");
	urldecode_test("http://www.baidu.com/paht1");
#endif
	RTSPClient *client = RTSPClient_open(url);
//开始接收视频流,
	if (client)
		RTSPClient_StreamRecv(client);
	printf("exit:%s\n", url);
	return 0;
}
