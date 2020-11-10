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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "base64.h"
#include <poll.h>
#include <errno.h>

#define DEBUG 1

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
	return -1;
}

struct URLInfo
{
	char prefix[10];
	char host[30];
	int port;
	char useranme[30];
	char password[30];
	const char *path;

	char Authorization[100];
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
***/
struct SDPInfo
{
	//m=video 0 RTP/AVP 96
	int media_video_port;
	int media_video_proto;
	int media_video_format;
	//a=rtpmap:96 H264/90000
	int media_video_attr_rtpmap_format;
	int media_video_attr_rtpmap_type; //H264
	int media_video_attr_rtpmap_rate; //90000
	//a=framerate:25.000000
	int media_video_framerate;
	//a=control:trackID=0
	int media_video_attr_trackID;
	//sps 大小in sprop-parameter-sets
};

typedef struct _RTSPClient
{
	char *url;
	int fd;
	struct URLInfo urlinfo;
//sdpinfo
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
	return send(fd, buf, strlen(buf), 0);
}

int socket_recv_text(int fd, char *buf, int len)
{
	return recv(fd, buf, len, 0);
}

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

	while (1)
	{
		ret = poll(&pofd, 1, 100);
		if (ret == 0)
			continue;
		if (ret > 0)
		{
			if (POLLIN == (pofd.revents & POLLIN))
			{
				ret = recv(fd, buff, sizeof(buff), 0);
				if (ret <= 0)
				{
					if (ret < 0 && (errno == EAGAIN || errno == EINTR))
						continue;
					break;
				}
				if (ret > 0)
				{
					fwrite(buff, ret, 1, fp);
					fflush(fp);
					//printf("membuf:%s, %d\n", membuf, memlen);
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
	fclose(fp);
	if (memlen == 0)
	{
		if (membuf != NULL)
		{
			free(membuf);
			membuf = NULL;
		}
	}

	*text = membuf;
	*textlen = memlen;

#if DEBUG
	printf("================\n%s\n=============\n", membuf);
#endif
	return 0;
}

int rtsp_DESCRIBE_base(int fd, struct URLInfo *urlinfo, char *authstr)
{
	char url[100];
	snprintf(url, sizeof(url), "DESCRIBE %s%s:%d%s RTSP/1.0\r\n", urlinfo->prefix, urlinfo->host, urlinfo->port, urlinfo->path);

	socket_send_text(fd, url);
	socket_send_text(fd, "CSeq: 1\r\n");
	socket_send_text(fd, "Accept: application/sdp\r\n");
	if (strlen(urlinfo->Authorization) > 0)
		socket_send_text(fd, urlinfo->Authorization);
	socket_send_text(fd, "User-Agent: RTSPClient1.0\r\n");
	socket_send_text(fd, "\r\n");

	char *resp_text = NULL;
	size_t resp_len = 0;
	int ret = rtsp_response_recv(fd, &resp_text, &resp_len);

	char pref[10];
	int code = 0;

	if (resp_text)
		sscanf(resp_text, "%[^ ] %d ", pref, &code);
	else
		code = -1;
	if (code == 401)
	{
		//需要加密发送哦
		const char *auth = strstr(resp_text, "WWW-Authenticate:");
		if (auth)
		{
			auth += strlen("WWW-Authenticate:");
			if (*auth == ' ')
				auth++;
			char *next = strchr(auth, '\r');
			if (next)
				*next = 0;
			strcpy(authstr, auth);
		}
	}

	if (code == 200)
	{
		//需要得到 sdp内容
		int head_len = string_http_heard_getlength(resp_text, strlen(resp_text));
		printf("--------SDP数据----------\n%s", resp_text + head_len);
		//m: media descriptions
		//a: attributes
	}
	if (resp_text)
		free(resp_text);
	return code;
}

int rtsp_DESCRIBE(int fd, struct URLInfo *urlinfo)
{
	int ret;
	char authstr[300];
	ret = rtsp_DESCRIBE_base(fd, urlinfo, authstr);
	if (ret == 401)
	{
		printf("auth:%s\n", authstr);
		if (strncmp(authstr, "Basic ", 6) == 0)
		{
			char pwd[100];
			char base64str[200];
			snprintf(pwd, sizeof(pwd), "%s:%s", urlinfo->useranme, urlinfo->password);
			base64_encode(pwd, strlen(pwd), base64str);
			sprintf(urlinfo->Authorization, "Authorization: Basic %s\r\n", base64str);
		}
		ret = rtsp_DESCRIBE_base(fd, urlinfo, authstr);
		if (ret == 401)
			return 401;
	}
	return ret;
}

RTSPClient *RTSPClient_open(const char *url)
{
	RTSPClient *client = (RTSPClient *) calloc(sizeof(RTSPClient), 1);
	if (client == NULL)
		return NULL;
	client->url = strdup(url);
	urldecode(client->url, &client->urlinfo);

	int fd = socket_tcp(client->urlinfo.host, client->urlinfo.port);
	if (fd == -1)
	{
		return NULL;
	}
	printf("connect success %d,[%s]\n", fd, client->urlinfo.host);
	client->fd = fd;
	rtsp_DESCRIBE(fd, &client->urlinfo);
	return client;
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
	urldecode_test(url);
#if 0
	urldecode_test("rtsp://admin:admin@11@192.168.0.150:554/paht1");
	urldecode_test("rtsp://admin:@192.168.0.150:554/paht1");
	urldecode_test("rtsp://192.168.0.150:554/paht1");
	urldecode_test("rtsp://:554/paht1");
	urldecode_test("rtsp:///paht1");
	urldecode_test("http://www.baidu.com/paht1");
#endif
	RTSPClient_open(url);
	return 0;
}
