﻿/*
 * Onvif_HttpDigest.c
 *
 *  Created on: 2019年11月7日
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#else
#include <sys/poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <unistd.h>
#endif

//#include "curl/curl.h"
#include <time.h>
#include <ctype.h>

#include "digest.h"
#include "client.h"
#include "SOAP_Onvif.h"

static uint32_t osclockms()
{
#ifdef _MSC_VER
	return GetTickCount64();
#else
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec * 1000 + tp.tv_nsec / 1000 / 1000;
#endif
}

static int http_content_point(const char *txt)
{
	const char *ptr = txt;
	while (ptr && *ptr)
	{
		if (*ptr == '\n')
		{
			if (*(ptr + 1) == '\r' || *(ptr + 1) == '\n')
				break;
		}
		ptr++;
	}
	if (ptr && *ptr)
	{
		while (*ptr)
		{
			if (*ptr != '\r' && *ptr != '\n')
			{
				break;
			}
			ptr++;
		}
	}
	return ptr - txt;
}

static void _cnonce_buildstr(char *randstr)
{
	int i = 0;
	char *ptr = randstr;
	unsigned short v = 0;
	srand((unsigned) time(NULL)); /*播种子*/
	memset(randstr, 0, 30);
	for (i = 0; i < 32; i += 2)
	{
		v = rand() % 100;/*产生随机整数*/
		sprintf(ptr, "%02X", v);
		ptr += 2;
	}
	*ptr = 0;
}

static int http_auth_digest(const char *dsc_disget, char *resultstr, int reslen,
			const char *username, const char *password, const char *uri)
{
	//https://github.com/jacketizer/libdigest
	digest_t d;
	char cnonce[33];
	const char *p_opaque = NULL;

	p_opaque = strstr(dsc_disget, "opaque=");

	if (-1 == digest_is_digest(dsc_disget))
	{
		fprintf(stderr, "Could not digest_is_digest!\n");
		return -1;
	}

	digest_init(&d);

	digest_client_parse(&d, dsc_disget);

	digest_set_attr(&d, D_ATTR_USERNAME, (digest_attr_value_tptr) username);
	digest_set_attr(&d, D_ATTR_PASSWORD, (digest_attr_value_tptr) password);
	digest_set_attr(&d, D_ATTR_URI, (digest_attr_value_tptr) uri);

	//char *cnonce = "400616322553302B623F0A0C514B0543";
	_cnonce_buildstr(cnonce);

	digest_set_attr(&d, D_ATTR_CNONCE, (digest_attr_value_tptr) cnonce);
	digest_set_attr(&d, D_ATTR_ALGORITHM, (digest_attr_value_tptr) 1);
	digest_set_attr(&d, D_ATTR_METHOD, (digest_attr_value_tptr) DIGEST_METHOD_POST);

	if (!p_opaque)
		digest_set_attr(&d, D_ATTR_OPAQUE, (digest_attr_value_tptr) "");

	if (-1 == digest_client_generate_header(&d, resultstr, reslen))
	{
		fprintf(stderr, "Could not build the Authorization header!\n");
	}
	return 0;
}

static int _socket_writestr(SOCKET fd, const char *str)
{
	return send(fd, str, strlen(str), 0);
}

//非阻塞模式
static inline int _socket_nonblock(SOCKET fd, int flag)
{
#ifdef _MSC_VER
	unsigned long ul = flag;
	return ioctlsocket(fd, FIONBIO, (unsigned long*)&ul);    //设置成非阻塞模式
#else
	int opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1)
	{
		return -1;
	}
	if (flag)
		opt |= O_NONBLOCK;
	else
		opt &= ~O_NONBLOCK;

	return fcntl(fd, F_SETFL, opt);
#endif
}

int socket_readline(int fd, char *line, int maxsize)
{
	char v;
	int ret;
	int count = 0;
	while (count < maxsize)
	{
		ret = recv(fd, &v, 1, 0);
		if (ret == 0)
		{
			return ret;
		}
		if (ret < 0)
		{
			if (errno == EAGAIN || errno == EINTR)
				continue;
			//WSAGetLastError
			return ret;
		}
		*line++ = v;
		count++;
		if (v == '\n' && count < maxsize)
		{
			*line = 0;
			break;
		}
	}
	return count;
}

int socket_checkin(SOCKET fd, int timeoutsec)
{
	fd_set readfds;
	struct timeval tv =
	{ timeoutsec, 0 };
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	select(((int) fd) + 1, &readfds, NULL, NULL, &tv);

	if (FD_ISSET(fd, &readfds))
	{
		return 1;
	}
	return 0;
}

static int http_post(const char *url, const char *authorization,
			const char *poststr, char **arg_http_resp, int *arg_resplen)
{
	char hoststr[100] = "";
	int port = 80;
	char *path = NULL;
	int ret;

	*arg_http_resp = NULL;
	*arg_resplen = 0;
	//只支持IP地址格式的
	//http://192.168.1.241.80:80/onvif/device_service
	//http://192.168.1.241.80/onvif/device_service
	//http://www.xxx.com:8888/onvif/device_service
	{
		char *ptr = NULL;
		char *tmpp = NULL;
		int isssl = 0;

		if (strncmp(url, "http://", 7) == 0)
		{
			isssl = 0;
			ptr = (char*) url + 7;
		}
		else if (strncmp(url, "https://", 8) == 0)
		{
			isssl = 1;
			ptr = (char*) url + 8;
		}
		tmpp = hoststr;
#if 1
		while (*ptr && tmpp != hoststr + 99)
		{
			//if (*ptr != '.' && !isdigit(*ptr))
			//	break;
			if (*ptr == ':' || *ptr == '/')
				break;
			*tmpp++ = *ptr++;
		}
		if (*ptr == ':')
		{
			port = atoi(ptr + 1);
			while (*ptr && *ptr != '/')
				ptr++;
		}
#endif
		path = ptr;
		if (!path || *path == 0)
			path = "/";
	}

	//printf("host:%s port:%d path:%s\n", hoststr, port, path);

	SOCKET fd;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	{
#if 0
		struct addrinfo ai, *res, *result;
		void *p = NULL;

		memset(&ai, 0, sizeof ai);
		ai.ai_family = PF_UNSPEC;
		ai.ai_socktype = SOCK_STREAM;
		ai.ai_flags = AI_CANONNAME;

		ret = getaddrinfo(hoststr, NULL, &ai, &result);
		if (ret)
		{
			fprintf(stderr, "getaddrinfo failed [%s]\n", hoststr);
			return -1;
		}
		if (ret == 0)
		{
			res = result;
			while (!p && res)
			{
				switch (res->ai_family)
				{
					case AF_INET:
						p = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
					break;
				}
				res = res->ai_next;
			}
		}
		if (!p)
		{
			freeaddrinfo(result);
			return -1;
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		//ret = inet_aton(hoststr, &addr.sin_addr);
		addr.sin_addr = *((struct in_addr *) p);
		freeaddrinfo(result);
#else
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(hoststr);

		if (addr.sin_addr.s_addr == INADDR_NONE)
		{
			struct hostent *host = gethostbyname(hoststr);
			if (host == NULL || host->h_addr== NULL)
			{
				return -1;
			}
			addr.sin_addr = *(struct in_addr*) host->h_addr;
		}

#endif
	}

	char buffcache[1024];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	ret = connect(fd, (struct sockaddr*) &addr, sizeof(addr));

	if (ret == -1)
	{
		printf("connect err:%d,%s, IP:%s:%d\n", errno, strerror(errno), hoststr,
					port);
		closesocket(fd);
		//goto _hquit;
		return -1;
	}

	sprintf(buffcache, "POST %s HTTP/1.1\r\n", path);
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "Host: %s\r\n", hoststr);
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "Accept: */*\r\n");
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "Content-Type: application/soap+xml; charset=utf-8\r\n");
	_socket_writestr(fd, buffcache);

	if (authorization && strlen(authorization) > 0)
	{
		_socket_writestr(fd, authorization);
		_socket_writestr(fd, "\r\n");
	}

	sprintf(buffcache, "Content-Length: %ld\r\n", strlen(poststr));
	_socket_writestr(fd, buffcache);

	_socket_writestr(fd, "\r\n");
	_socket_writestr(fd, poststr);

	//struct pollfd pos;
	//pos.fd = fd;
	//pos.events = POLLIN | POLLHUP | POLLERR | POLLNVAL;

	int content_length = -1;
	int http_head_readflag = 0;
	int http_code = -1;

	_socket_nonblock(fd, 1); //非阻塞

//	void *mmeptr=NULL;
//	size_t memsize=0;
//	FILE *fptmp = open_memstream(&mmeptr, &memsize);
	FILE *fptmp = tmpfile();
	long int filesize = 0;
	long int http_head_size = 0;
	int loopflag = 1;

	while (loopflag)
	{
		if (filesize > 1024 * 1024 * 2)
		{
			perror("不能超过2M\n");
			break;
		}

		ret = socket_checkin(fd, 1);

		//ret = poll(&pos, 1, 2 * 1000);
		//if (ret <= 0)
		//{
		//	printf("timeout ret=%d\n", ret);
		//	break;
		//}
		//if (pos.revents != POLLIN)
		//	continue;
		//read http head
		if (0 == http_head_readflag)
		{
			while (!http_head_readflag)
			{
				ret = socket_readline(fd, buffcache, sizeof(buffcache));
				if (ret <= 0)
				{
					loopflag = 0;
					break;
				}
				fprintf(fptmp, "%s", buffcache);
				//fputs(buffcache,fptmp);
				filesize = ftell(fptmp);
				if (http_code == -1)
				{
					ret = sscanf(buffcache, "%*[^ ]%d", &http_code);
					if (ret <= 0)
					{
						http_code = 0;
					}
				}
				if (strncasecmp(buffcache, "Content-Length:",
							strlen("Content-Length:")) == 0)
				{
					sscanf(buffcache, "%*[^0-9]%d", &content_length);
				}

				if ((buffcache[0] == '\r' && buffcache[1] == '\n')
							|| buffcache[0] == '\n')
				{
					http_head_readflag = 1;
					break;
				}
			} //end while
			http_head_size = filesize;
			if (0 == content_length)
				break;
			if (401 == http_code && -1 == content_length)
				break;
			continue;
		}

		{
			//memset(buffcache,0,sizeof(buffcache));
			ret = recv(fd, buffcache, sizeof(buffcache), 0);
			if (ret == 0)
			{
				printf("ret=0\n");
				break;
			}
			if (ret < 0)
			{
				if (errno == EAGAIN || errno == EINTR)
					continue;
				break;
			}

			fwrite(buffcache, 1, ret, fptmp);
			filesize = ftell(fptmp);

			if (content_length >= 0)
			{
				content_length -= ret;
				if (content_length <= 0)
					break;
			}
		}
	}

	_hquit:

	//shutdown(fd, SHUT_RDWR);
	closesocket(fd);

	//printf("http size:%ld\n", filesize);

	long httpcode = 0;

	if (filesize > 0)
	{
		fseek(fptmp, 0, SEEK_SET); //rewind(fptmp);
		fscanf(fptmp, "%*9s %ld", &httpcode);
		ret = httpcode;

		if (httpcode == 200)
		{ //去掉http头
			fseek(fptmp, http_head_size, SEEK_SET);

			filesize -= http_head_size;
			*arg_http_resp = (char*) malloc(filesize + 1); //\0
			(*arg_http_resp)[filesize] = 0;
			*arg_resplen = filesize;

			fread(*arg_http_resp, 1, filesize, fptmp);
		}
		else
		{
			*arg_http_resp = (char*) malloc(filesize + 1); //\0
			(*arg_http_resp)[filesize] = 0;
			*arg_resplen = filesize;
			fread(*arg_http_resp, 1, filesize, fptmp);
		}
	}
	fclose(fptmp);
	return ret;
}

#ifdef CONFIG_USE_CURL_HTTP_POST

struct data_buff
{
	char *buff;
	int size;
	int index;
};

static size_t _http_curl_wheader(char* buffer, size_t size, //大小
			size_t nitems,//哪一块
			struct data_buff *out)
{
	//strncat(out, buffer, size * nitems);
	memcpy(out->buff + out->index, buffer, size * nitems);
	out->index += size * nitems;
	return size * nitems;
}

static size_t _http_curl_wcontent(char* buffer, size_t size, //大小
			size_t nitems,//哪一块
			struct data_buff *out)
{
	int len = size * nitems;
	if (len + out->index < out->size)
	{
		memcpy(out->buff + out->index, buffer, len);
		out->index += len;
	}
	return size * nitems;
}

static int http_post(const char *url, const char *authorization,
			const char *poststr, char *respstr, int resplen)
{
	CURL* curl;
	CURLcode res;
	struct curl_slist* http_header = NULL;
	struct data_buff buff;

	buff.buff = respstr;
	buff.size = resplen;
	buff.index = 0;

	http_header = curl_slist_append(http_header,
				"Content-Type: application/soap+xml; charset=utf-8");
	if (authorization)
	http_header = curl_slist_append(http_header, authorization);

	// 初始化CURL
	curl = curl_easy_init();
	if (!curl)
	{
		perror("curl init failed \n");
		return -1;
	}

	// 设置CURL参数
	//curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	curl_easy_setopt(curl, CURLOPT_URL, url);//url地址
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, poststr);//post参数
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(poststr));

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_curl_wheader);//处理http头部
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &buff);//http头回调数据

	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  //将响应头信息和相应体一起传给write_data

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_curl_wcontent);//处理http内容
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buff);//这是write_data的第四个参数值

	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, resplen - 1);
	curl_easy_setopt(curl, CURLOPT_POST, 1);//设置问非0表示本次操作为post
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息

	//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
	//curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/tmp/curlpost.cookie");

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);//接收数据时超时设置，如果10秒内数据未接收完，直接退出
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0); //设为false 下面才能设置进度响应函数

	res = curl_easy_perform(curl);

	long httpcode = 0;
	long headersize = 0;
	curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &headersize);

	// 判断是否接收成功
	if (res != CURLE_OK)
	{ //CURLE_OK is 0 CURLE_OPERATION_TIMEDOUT
		fprintf(stderr, "-->CURL err: %s,%d \n", url, res);
		printf("[headersize:%ld] res=%d\n", headersize, res);
	}
	if (headersize > 0)
	{
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcode);
		res = httpcode;
	}

	if (httpcode == 200)
	{
		//int sumlen = strlen(respstr);
		int cpoit = http_content_point(respstr);
		if (cpoit < resplen)
		memcpy(respstr, respstr + cpoit, resplen - cpoit);
	}

	// 释放CURL相关分配内存
	curl_slist_free_all(http_header);
	curl_easy_cleanup(curl);
	return res;
}
#endif

/**
 * onvif的http请求, return 200成功
 *
 //Authorization: Digest username="admin", realm="Login to 2J01018PAA00813",
 //qop="auth", algorithm="MD5", uri="/onvif/device_service", nonce="b252aWYtZGlnZXN0OjQzNTIyMDY3OTEw",
 //nc=00000001, cnonce="AD484FC8EC7964EDA1EC6F89FC8A93CC", opaque="", response="6433f6224f6efe9a6102afcdc707b5ec"

 //Authorization: Digest username="admin", realm="DS-2CD3T20D-I3", qop="auth", algorithm="MD5",
 //uri="/onvif/device_service", nonce="4d6b4a434d304d34526a5936595449324e4442684d44633d", nc=00000001,
 //cnonce="EA28623C85D04E3E990D062537937287", response="cca6532025b67e5bb9435c6b9746dbfe"

 //Authorization: Digest username="admin", realm="DS-2CD3T20D-I3", qop="auth", algorithm="MD5",
 //nonce="4d3051324d44633152455936595449324d54597a596d453d", nc=00000001, cnonce="0613103F5362041E462955510E545511",
 //opaque="(null)", response="09a29f7b1e9f5b741fe2ff7e8354fe04"  错误的
 */
int onvif_http_digest_post(const char *url, const char *username,
			const char *password, const char *postStr, char **pxml, int *xmllen)
{
	char *authenticate401 = NULL, *ptr = NULL;
	char authorization[1024];
	char uri[100];
	int ret = 0;

	char *arg_http_resp = NULL;
	int arg_resplen;

	uint32_t tmcounts[2];

	memset(authorization, 0, sizeof(authorization));

	*pxml = NULL;
	*xmllen = 0;
	tmcounts[0] = osclockms();

	ret = http_post(url, NULL, postStr, &arg_http_resp, &arg_resplen);

	if (ret == 401)
	{
		if (NULL != username && NULL != password)
		{
			authenticate401 = strstr(arg_http_resp, "WWW-Authenticate:");
			if (authenticate401)
				authenticate401 = strchr(authenticate401, ':');

			if (authenticate401)
			{
				authenticate401++;
				while (authenticate401 && *authenticate401 == ' '
							&& *authenticate401 != '\n' && *authenticate401 != '\r'
							&& *authenticate401 != 0)
					authenticate401++;

				ptr = authenticate401;
				while (ptr && *ptr)
				{
					if (*ptr == '\r' || *ptr == '\n')
					{
						*ptr = 0;
						break;
					}
					ptr++;
				}
				///
				int namelen = strlen("Authorization: ");
				sprintf(authorization, "Authorization: ");

				{
					char *next = strstr(url, "://");
					memset(uri, 0, sizeof(uri));
					if (next)
					{
						next += 3;
						next = strchr(next, '/');
						if (next)
						{
							strncpy(uri, next, sizeof(uri) - 1);
						}
					}
				}
				//printf("authenticate401:%s\n", authenticate401);
				ret = http_auth_digest(authenticate401, authorization + namelen,
							sizeof(authorization) - namelen, username, password,
							uri);

				if (arg_http_resp)
					free(arg_http_resp);
				arg_http_resp = NULL;
				arg_resplen = 0;
				ret = http_post(url, authorization, postStr, &arg_http_resp,
							&arg_resplen);
			}
		}
	}  //401

//	if (arg_http_resp) {
//		strncpy(xml, arg_http_resp, arg_resplen);
//		free(arg_http_resp);
//	}
	if (arg_http_resp)
	{
		*pxml = arg_http_resp;
		*xmllen = arg_resplen;
	}

	tmcounts[1] = osclockms();
	//printf("http time:%ld\n", tmcounts[1] - tmcounts[0]);
	return ret;
}

#define XML_GetDeviceInformation \
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
		"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
		"<GetDeviceInformation xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>"\
		"</s:Body></s:Envelope>"

//<tds:GetDeviceInformationResponse>
//  <tds:Manufacturer>Dahua</tds:Manufacturer>
//  <tds:Model>IPC-HDW1025C</tds:Model>
//  <tds:FirmwareVersion>2.420.Dahua 00.14.R, build: 2016-06-18</tds:FirmwareVersion>
//  <tds:SerialNumber>2J01018PAA00813</tds:SerialNumber>
//  <tds:HardwareId>1.00</tds:HardwareId>
//</tds:GetDeviceInformationResponse>

struct xml_node
{
	char *name;
	char *text;
};

static struct xml_node* xml_node_alloc()
{
	return (struct xml_node*) calloc(sizeof(struct xml_node), 1);
}

static void xml_node_Delete(struct xml_node *node)
{
	if (node)
	{
		if (node->name)
			free(node->name);
		if (node->text)
			free(node->text);
		free(node);
	}
}

static char* strchr_end(const char *str_head, char *str_end, int c)
{
	while (str_end >= str_head)
	{
		if (*str_end == c)
			return str_end;
		str_end--;
	}
	return NULL;
}

static struct xml_node* xml_getNode(const char *xmlstr, const char *name)
{
	char *ps = NULL;
	char *ps_end = NULL;
	struct xml_node *node = NULL;

	ps = strstr(xmlstr, name);
	if (ps && *ps != '<')
	{
		ps = strchr_end(xmlstr, ps, '<');
		if (ps)
			ps++;
	}
	if (!ps)
		return NULL;

	ps_end = strchr(ps, '>');
	if (ps_end == NULL)
		return NULL;

	int len = ps_end - ps;
	ps_end = strstr(ps, "</");
	if (!ps_end)
		return NULL;
	node = xml_node_alloc();
	node->name = (char*) malloc(len);
	strncpy(node->name, ps, len - 1);

	ps += len + 1;
	len = ps_end - ps + 1;
	node->text = (char*) malloc(len);
	node->text[len - 1] = 0;
	strncpy(node->text, ps, len - 1);
	return node;
}

int Onvif_GetDeviceInformation(const char *url, const char *user,
			const char *pass, struct OnvifDeviceInformation *info)
{
	int ret = 0;
	struct xml_node *node;
	char *xmlstr = NULL;
	int xmllen = 0;

	ret = onvif_http_digest_post(url, user, pass, XML_GetDeviceInformation,
				&xmlstr, &xmllen);
	if (ret == 200 && xmlstr)
	{
		node = xml_getNode(xmlstr, "Manufacturer>");
		if (node)
		{
			strncpy(info->Manufacturer, node->text, sizeof(info->Manufacturer));
		}
		xml_node_Delete(node);
		node = xml_getNode(xmlstr, "Model>");
		if (node)
		{
			strncpy(info->Model, node->text, sizeof(info->Model));
		}
		xml_node_Delete(node);
		node = xml_getNode(xmlstr, "FirmwareVersion>");
		if (node)
		{
			strncpy(info->FirmwareVersion, node->text,
						sizeof(info->FirmwareVersion));
		}
		xml_node_Delete(node);
		node = xml_getNode(xmlstr, "SerialNumber>");
		if (node)
		{
			strncpy(info->SerialNumber, node->text, sizeof(info->SerialNumber));
		}
		xml_node_Delete(node);
		node = xml_getNode(xmlstr, "HardwareId>");
		if (node)
		{
			strncpy(info->HardwareId, node->text, sizeof(info->HardwareId));
		}
		xml_node_Delete(node);
	}
	if (xmlstr)
		free(xmlstr);
	info->httpRespStatus = ret;
	return ret;
}
#define XML_GetCapabilities_Media "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\">"\
	"<Category>Media</Category>"\
	"</GetCapabilities>"\
	"</s:Body></s:Envelope>"

int Onvif_GetCapabilities_Media(const char *url, const char *user,
			const char *pass, char *mediaUrl)
{
	int ret = 0;
	struct xml_node *node;
	char *xmlstr = NULL;
	int xmllen = 0;

	ret = onvif_http_digest_post(url, user, pass, XML_GetCapabilities_Media,
				&xmlstr, &xmllen);
	if (ret == 200 && xmlstr)
	{
		char *xmlMedia = strstr(xmlstr, "Media>");
		if (xmlMedia)
		{
			node = xml_getNode(xmlMedia, "XAddr>");
			if (node)
			{
				strcpy(mediaUrl, node->text);
			}
			xml_node_Delete(node);
		}
	}
	if (xmlstr)
		free(xmlstr);
	return ret;
}

int printstr(const char *p, int len)
{
	const char *end = p + len;
	int ret = 0;
	while (p < end)
	{
		putchar(*p++);
		ret++;
	}
	return ret;
}

static char* xml_findend(const char *xmlstr, const char *labelname,
			int labellen)
{
	const char *p = xmlstr;
	const char *end = xmlstr + strlen(xmlstr);
	while (p < end)
	{
		if (*p == '<' && *(p + 1) == '/')
		{
			p += 2;
			if (strncmp(p, labelname, labellen) == 0)
			{
				return (char*) p - 2;
			}
		}
		p++;
	}
	return NULL;
}

void xml_label_out(const char *xmllabel, int llen, const char *txt, int tlen,
			void *user)
{
	printf("label:");
	printstr(xmllabel, llen);
	printf("\n");

	printf(" txt:");
	if (txt[0] != '<')
		printstr(txt, tlen);
	else
		printf("[xml format]");
	printf("\n");
}

void xml_label(const char *xmlstr,
			void (*callback)(const char *xmllabel, int llen, const char *txt,
						int tlen, void *user), void *user)
{
	const char *p = xmlstr;
	const char *end = xmlstr + strlen(xmlstr);
	char *lend = NULL;
	const char *txt_s = NULL;
	const char *txt_e = NULL;
	const char *lname_end;
	const char *lname_s;

	while (p && p < end)
	{
		//xml 开始

		if (p + 1 < end && *p == '<' && *(p + 1) != '/')
		{
			p++;

			lend = strchr(p, '>');
			if (lend)
			{
				if (*(lend - 1) == '/')
				{
					lend--;
				}
				lname_end = strchr(p, ' ');
				if (lname_end > lend || !lname_end)
					lname_end = lend;
				lname_s = p;

				if (*lend == '/')
				{
					txt_s = "";
					txt_e = NULL;
				}
				else
				{
					txt_s = lend + 1;
					txt_e = xml_findend(txt_s, lname_s, lname_end - lname_s);
					if (txt_e == NULL)
					{
						txt_e = end;
					}
				}
				//printf("label:");
				//if(0==printstr(lname_s, lname_end - lname_s)){
				//	printf("   [%.10s]\n", lname_s);
				//}
				//printf("\n");
				*lend = 0;
				if (callback)
				{
					callback(p, lend - p, txt_s, txt_e - txt_s, user);
				}
				//xml_label_out(p, lend - p, txt_s, txt_e - txt_s);
				p = lend;
			}
			else
			{
				fprintf(stderr, "length not find '>' %s\n", p);
			}
			continue;
		}
		p++;
	}
}

void xml_label2(const char *xmlstr,
			void (*callback)(int treeindex, const char *nodename, int namelen, const char *txt, int txtlen, void *ctx),
			void *ctx)
{
	const char *p = xmlstr; //xml 文档开始
	const char *end = xmlstr + strlen(xmlstr); //文档结尾
	const char *lend = NULL; //元素节点 结束位置
	const char *txt_s = NULL; //元素内容开始
	const char *txt_e = NULL; //元素内容结束

	const char *lname_s; //元素名 开始位置
	const char *lname_end; //元素名  结束位置
	const char *node_end; //元素  结束位置
	const char *node_start; //元素 开始位置
	int namelen = 0;

	//层次
	while (p && p < end)
	{
		if (p + 1 < end && *p == '<' && *(p + 1) != '/') //寻找<总是一一对应的 <xx:a> </xx:a>  <xx/>
		{
			p++;
			node_start = p;
			lend = strchr(p, '>');  //寻找一个元素节点  p:<   lend:>
			if (lend)
			{
				lname_s = p;
				txt_s = lend + 1; //元素内容开始位置

				if (*(lend - 1) == '/' || *(lend - 1) == '?')  //如果为 /> 表示没有元素内容
				{
					lend--;
					txt_s = NULL;  //元素内容为空
					txt_e = NULL;
				}
				node_end = lend;

				lname_end = strchr(p, ' ');
				if (lname_end > lend || !lname_end)
					lname_end = lend;   // <name/> 这样的类型

				namelen = lname_end - lname_s; //表示元素name长度

				if (txt_s)
				{ //这是存在元素内容
					//寻找元素结尾, 注意 <?xml version="1.0" encoding="UTF-8"?>
					while (*txt_s == '\n' || *txt_s == '\r' || *txt_s == ' ')
						txt_s++;
					txt_e = txt_s;
					while (txt_e != end)
					{
						if (*txt_e == '<' && *(txt_e + 1) == '/' && strncmp(txt_e + 2, lname_s, namelen) == 0)
							break;
						txt_e++;
					}
					if (txt_e == end)
						txt_e = txt_s;					//没找到, 长度为0

				}

				if (callback)
					callback(0, node_start, node_end - node_start, txt_s, txt_e - txt_s, ctx);

				p = lend;  //转到下一个位置
			}
			else
			{
				fprintf(stderr, "length not find '>' %s\n", p);
			}
			continue;
		}
		p++;
	}
}

const char* strchr_len(const char *ptr, const int v, int len)
{
	const char *end = ptr + len;
	while (ptr < end)
	{
		if (*ptr == v)
			return ptr;
		ptr++;
	}
	return NULL;
}

const char* strstrend(const char *str, const char *end, const char *value)
{
	while (str < end - strlen(value))
	{
		if (strncmp(str, value, strlen(value)) == 0)
			return str;
		str++;
	}
	return NULL;
}

#define XML_GetProfiles "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>"\
	"</s:Body></s:Envelope>"

#define XML_LABEL_Profiles  "Profiles"   // <trt:Profiles fixed="true" token="profile_token_1">
#define XML_LABEL_Bounds	"Bounds"
#define XML_LABEL_Encoding  "Encoding"
#define XML_LABEL_Name		"Name"
#define XML_LABEL_VideoEncoderConfiguration			"VideoEncoderConfiguration"

struct OnvifGetProfilesResContext
{
	struct OnvifDeviceProfiles *profiles;
	int index;
	int profsize;
};

static void _xml_GetProfilesResponse_Profiles_size(int treeindex, const char *node_start, int nodelen, const char *txt, int txtlen, void *ctx)
{
	const char *end = node_start + nodelen;
	int namelen = 0;
	const char *name_start = strchr_len(node_start, ':', nodelen);
	const char *name_end = NULL;
	const char *attrib_ptr = NULL;

	if (name_start)
		name_start++;
	name_start = name_start ? name_start : node_start;
	name_end = strchr_len(node_start, ' ', nodelen);
	name_end = name_end ? name_end : end;
	namelen = name_end - name_start;

	int *_size = (int*) ctx;
	//寻找结尾点
	if (namelen > 0 && strncasecmp(XML_LABEL_Profiles, name_start, namelen) == 0)
		(*_size)++;
}

static void _xml_GetProfilesResponse(const char *xmllabel, int llen,
			const char *txt, int tlen, void *user)
{
	struct OnvifGetProfilesResContext *context =
				(struct OnvifGetProfilesResContext*) user;

	const char *label = strchr(xmllabel, ':');
	char *attrib = (char*) xmllabel;

	if (label)
	{
		label++;
	}
	else
	{
		label = xmllabel;
	}
	while (*attrib)
	{
		attrib++;
		if (*attrib == ' ')
			break;
	}
	if (attrib)
	{
		*attrib = 0;  //这里会修改xml的数据哦
		attrib++;
	}
	//printf("label:%s\n", label);
	if (strcasecmp(XML_LABEL_Profiles, label) == 0)
	{
		//printf("label:%s,attrib:%s\n", label, attrib);
		if (strstr(attrib, "token="))
		{
			char token[100];
			char *p = strstr(attrib, "token=");
			if (p)
				sscanf(p, "token=%*c%[^\"\']", token);
			else
				token[0] = 0;

			if (context->index <= context->profsize)
			{
				strcpy(context->profiles[context->index].token, token);
				context->index++;
			}
		}
	}
	//  <tt:Bounds x="0" y="0" width="2048" height="1536" />
#if 0
	if (strcasecmp(XML_LABEL_Bounds, label) == 0)
	{
		int width = 0, height = 0;
		const char* ptr = NULL;

		if (strstr(attrib, "width"))
		{
			ptr = strstr(attrib, "width");
			sscanf(ptr, "width=%*c%d%*c", &width);
		}
		if (strstr(attrib, "height"))
		{
			ptr = strstr(attrib, "height");
			sscanf(ptr, "height=%*c%d%*c", &height);
		}
		if (context->index > 0 && context->index < context->profsize)
		{
			context->profiles[context->index - 1].width = width;
			context->profiles[context->index - 1].height = height;
		}
	}
	/***
	<tt:VideoEncoderConfiguration token="VideoEncode_token_3">
        <tt:Name>VideoEncode_3</tt:Name>
        <tt:UseCount>1</tt:UseCount>
        <tt:Encoding>H264</tt:Encoding>
        <tt:Resolution>
            <tt:Width>704</tt:Width>
            <tt:Height>480</tt:Height>
        </tt:Resolution>
        <tt:Quality>3</tt:Quality>
        <tt:RateControl>
            <tt:FrameRateLimit>20</tt:FrameRateLimit>
            <tt:EncodingInterval>1</tt:EncodingInterval>
            <tt:BitrateLimit>480</tt:BitrateLimit>
        </tt:RateControl>
        <tt:H264>
            <tt:GovLength>80</tt:GovLength>
            <tt:H264Profile>High</tt:H264Profile>
        </tt:H264>
        <tt:Multicast>
            <tt:Address>
                <tt:Type>IPv4</tt:Type>
                <tt:IPv4Address>239.0.0.2</tt:IPv4Address>
            </tt:Address>
            <tt:Port>52554</tt:Port>
            <tt:TTL>1</tt:TTL>
            <tt:AutoStart>false</tt:AutoStart>
        </tt:Multicast>
        <tt:SessionTimeout>PT60S</tt:SessionTimeout>
    </tt:VideoEncoderConfiguration>
	**/
#endif
	//<tt:Encoding>H264</tt:Encoding>
	if (strcasecmp(XML_LABEL_Encoding, label) == 0)
	{
		if (txt)
			((char*) txt)[tlen] = 0;

		if (strlen(context->profiles[context->index - 1].encodname) == 0
					&& context->index > 0
					&& context->index <= context->profsize)
			strncpy(context->profiles[context->index - 1].encodname, txt, 20 - 1);
	}

	if (strcasecmp("Width", label) == 0 && txt)
	{
		if (txt)
			((char*) txt)[tlen] = 0;
		if (context->index > 0 && context->index <= context->profsize)
			sscanf(txt, "%d", &context->profiles[context->index - 1].width);
	}
	if (strcasecmp("Height", label) == 0 && txt)
	{
		if (txt)
			((char*) txt)[tlen] = 0;
		if (context->index > 0 && context->index <= context->profsize)
			sscanf(txt, "%d", &context->profiles[context->index - 1].height);
	}
	if (strcasecmp("FrameRateLimit", label) == 0 && txt) //帧率
	{
		if (context->index > 0 && context->index <= context->profsize)
			sscanf(txt, "%d", &context->profiles[context->index - 1].frameRateLimit);
	}
	if (strcasecmp("BitrateLimit", label) == 0 && txt) //码率
	{
		if (context->index > 0 && context->index <= context->profsize)
			sscanf(txt, "%d", &context->profiles[context->index - 1].bitrateLimit);
	}
	if (strcasecmp(XML_LABEL_Name, label) == 0)
	{
		if (txt)
		{
			((char*) txt)[tlen] = 0;
			//strncpy(context->profiles[context->index - 1].encodname, txt,
			//		20 - 1);
		}
		//printf("name:%s\n", txt);
	}
}

int Onvif_MediaService_GetProfiles(const char *mediaUrl, const char *user,
			const char *pass,
			struct OnvifDeviceProfiles **profiles,
			int *profsize)
{
	int ret = 0;
	char *xmlstr = NULL;
	int xmllen = 0;

	struct OnvifGetProfilesResContext context;
	context.profiles = NULL;
	context.index = 0;
	context.profsize = 0;
	ret = onvif_http_digest_post(mediaUrl, user, pass, XML_GetProfiles, &xmlstr,
				&xmllen);
	if (ret == 200 && xmlstr)
	{
		//首先需要统计Profiles的数量
		int _Profiles_size = 0;
		xml_label2(xmlstr, _xml_GetProfilesResponse_Profiles_size, &_Profiles_size);
		if (_Profiles_size > 0)
		{
			context.profiles = (struct OnvifDeviceProfiles*) calloc(_Profiles_size, sizeof(struct OnvifDeviceProfiles));
			if (context.profiles)
			{
				*profsize = _Profiles_size;
				context.profsize = _Profiles_size;
				xml_label(xmlstr, _xml_GetProfilesResponse, &context);

				*profiles = context.profiles;
				context.profiles[0].httpRespStatus = ret;
			}
		}
	}
	if (xmlstr)
		free(xmlstr);
	return ret;
}

#define XML_GetStreamUri "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">"\
	"<StreamSetup>"\
	"<Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream>"\
	"<Transport xmlns=\"http://www.onvif.org/ver10/schema\">"\
	"<Protocol>UDP</Protocol>"\
	"</Transport>"\
	"</StreamSetup>"\
	"<ProfileToken>%s</ProfileToken>"\
	"</GetStreamUri></s:Body>"\
	"</s:Envelope>"

static void xml_GetStreamUriResponse(const char *xmllabel, int llen,
			const char *txt, int tlen, void *user)
{
	const char *label = strchr(xmllabel, ':');
	char *attrib = (char*) xmllabel;

	if (label)
	{
		label++;
	}
	else
	{
		label = xmllabel;
	}
	while (*attrib)
	{
		attrib++;
		if (*attrib == ' ')
			break;
	}
	if (attrib)
	{
		*attrib = 0;
		attrib++;
	}

	if (strcmp("Uri", label) == 0)
	{
		((char*) txt)[tlen] = 0;
		strncpy((char*) user, txt, 300);
	}
}

int Onvif_MediaServer_GetStreamUri(const char *url, const char *user,
			const char *pass, const char *ProfileToken, char uri[300])
{
	int ret = 0;
	char str[1024];
	char *xmlstr = NULL;
	int xmllen = 0;

	memset(str, 0, sizeof(str));
	snprintf(str, sizeof(str) - 1, XML_GetStreamUri, ProfileToken);

	ret = onvif_http_digest_post(url, user, pass, str, &xmlstr, &xmllen);
	if (ret == 200 && xmlstr)
	{
		xml_label(xmlstr, xml_GetStreamUriResponse, uri);
	}
	if (xmlstr)
		free(xmlstr);
	return ret;
}

void OnvifURI_Decode(const char *url, char *desc, int dlen)
{
#define URL_HEAD_RTSP "rtsp://"
	const char *ptr = NULL;
	const char *end = desc + dlen;

	strncpy(desc, url, dlen);

	if (strncasecmp(url, URL_HEAD_RTSP, strlen(URL_HEAD_RTSP)) != 0)
		return;
	//https://blog.csdn.net/haydroid/article/details/46380069
	//去除&amp;
#define URL_AMP "amp;"
	ptr = url;

	while (desc < end && *ptr)
	{
		*desc++ = *ptr;
		if (*ptr == '&')
		{
			if (strncmp(ptr + 1, URL_AMP, strlen(URL_AMP)) == 0)
			{
				ptr += strlen(URL_AMP) + 1;
				continue;
			}
		}
		ptr++;
	}
	if (desc < end)
		*desc = 0;
}

#if 0
int Onvif_HttpTest(const char *url, const char *username, const char *passowrd)
{
	int ret;
	char mediaUrl[100];
	struct OnvifDeviceInformation info;
	memset(&info, 0, sizeof(info));

//	char xmlstr[] =
//			"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsdd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" xmlns:tpa=\"http://www.onvif.org/ver10/pacs\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:tac=\"http://www.onvif.org/ver10/accesscontrol/wsdl\" xmlns:tad=\"http://www.onvif.org/ver10/analyticsdevice/wsdl\" xmlns:tae=\"http://www.onvif.org/ver10/actionengine/wsdl\" xmlns:tana=\"http://www.onvif.org/ver20/analytics/wsdl/AnalyticsEngineBinding\" xmlns:tanr=\"http://www.onvif.org/ver20/analytics/wsdl/RuleEngineBinding\" xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" xmlns:tar=\"http://www.onvif.org/ver10/accessrules/wsdl\" xmlns:tasa=\"http://www.onvif.org/ver10/advancedsecurity/wsdl/AdvancedSecurityServiceBinding\" xmlns:tasd=\"http://www.onvif.org/ver10/advancedsecurity/wsdl/Dot1XBinding\" xmlns:task=\"http://www.onvif.org/ver10/advancedsecurity/wsdl/KeystoreBinding\" xmlns:tast=\"http://www.onvif.org/ver10/advancedsecurity/wsdl/TLSServerBinding\" xmlns:tas=\"http://www.onvif.org/ver10/advancedsecurity/wsdl\" xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns:tec=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\" xmlns:tee=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\" xmlns:tenc=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\" xmlns:tenp=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\" xmlns:tep=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\" xmlns:tepa=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:tepu=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:tesm=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" xmlns:tls=\"http://www.onvif.org/ver10/display/wsdl\" xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" xmlns:tndl=\"http://www.onvif.org/ver10/network/wsdl/DiscoveryLookupBinding\" xmlns:tdn=\"http://www.onvif.org/ver10/network/wsdl\" xmlns:tnrd=\"http://www.onvif.org/ver10/network/wsdl/RemoteDiscoveryBinding\" xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" xmlns:trc=\"http://www.onvif.org/ver10/recording/wsdl\" xmlns:trp=\"http://www.onvif.org/ver10/replay/wsdl\" xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" xmlns:trt2=\"http://www.onvif.org/ver20/media/wsdl\" xmlns:trv=\"http://www.onvif.org/ver10/receiver/wsdl\" xmlns:tse=\"http://www.onvif.org/ver10/search/wsdl\" xmlns:tss=\"http://www.onvif.org/ver10/schedule/wsdl\" xmlns:tth=\"http://www.onvif.org/ver10/thermal/wsdl\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\" xmlns:ter=\"http://www.onvif.org/ver10/error\"><SOAP-ENV:Body><trt:GetProfilesResponse><trt:Profiles fixed=\"true\" token=\"profile_token_1\"><tt:Name>profile1</tt:Name><tt:VideoSourceConfiguration token=\"VideoSource_token_1\"><tt:Name>VideoSource_1</tt:Name><tt:UseCount>4</tt:UseCount><tt:SourceToken>VideoSource_token_1</tt:SourceToken><tt:Bounds height=\"1080\" width=\"1920\" y=\"0\" x=\"0\"></tt:Bounds></tt:VideoSourceConfiguration><tt:AudioSourceConfiguration token=\"AudioSource_token_1\"><tt:Name>AudioSource_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:SourceToken>AudioSource_token_1</tt:SourceToken></tt:AudioSourceConfiguration><tt:VideoEncoderConfiguration token=\"VideoEncode_token_1\"><tt:Name>VideoEncode_1</tt:Name><tt:UseCount>1</tt:UseCount><tt:Encoding>H264</tt:Encoding><tt:Resolution><tt:Width>1920</tt:Width><tt:Height>1080</tt:Height></tt:Resolution><tt:Quality>5</tt:Quality><tt:RateControl><tt:FrameRateLimit>25</tt:FrameRateLimit><tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>3072</tt:BitrateLimit></tt:RateControl><tt:H264><tt:GovLength>100</tt:GovLength><tt:H264Profile>High</tt:H264Profile></tt:H264><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.0</tt:IPv4Address></tt:Address><tt:Port>50554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:VideoEncoderConfiguration><tt:AudioEncoderConfiguration token=\"AudioEncode_token_1\"><tt:Name>AudioEncode_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:Encoding>G711</tt:Encoding><tt:Bitrate>128</tt:Bitrate><tt:SampleRate>8</tt:SampleRate><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.3</tt:IPv4Address></tt:Address><tt:Port>53554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:AudioEncoderConfiguration><tt:VideoAnalyticsConfiguration token=\"VideoAnalytics0\"><tt:Name>MotionAnalytics_0</tt:Name><tt:UseCount>4</tt:UseCount><tt:AnalyticsEngineConfiguration><tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"CellMotionModule\"><tt:Parameters><tt:SimpleItem Value=\"42\" Name=\"Sensitivity\"></tt:SimpleItem><tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.001042\" y=\"0.001852\"/></tt:Transformation></tt:CellLayout></tt:ElementItem></tt:Parameters></tt:AnalyticsModule></tt:AnalyticsEngineConfiguration><tt:RuleEngineConfiguration><tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MotionDetectorRule\"><tt:Parameters><tt:SimpleItem Value=\"1\" Name=\"MinCount\"></tt:SimpleItem><tt:SimpleItem Value=\"0\" Name=\"AlarmOnDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"20000\" Name=\"AlarmOffDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"0P8A8A==\" Name=\"ActiveCells\"></tt:SimpleItem></tt:Parameters></tt:Rule></tt:RuleEngineConfiguration></tt:VideoAnalyticsConfiguration><tt:PTZConfiguration token=\"PTZToken\"><tt:Name>PTZconfig</tt:Name><tt:UseCount>3</tt:UseCount><tt:NodeToken>PTZToken</tt:NodeToken><tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace><tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace><tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace><tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace><tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace><tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace><tt:DefaultPTZSpeed><tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"0.100000001\" x=\"0.100000001\"></tt:PanTilt><tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom></tt:DefaultPTZSpeed><tt:DefaultPTZTimeout>PT10S</tt:DefaultPTZTimeout><tt:PanTiltLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:Range></tt:PanTiltLimits><tt:ZoomLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:Range></tt:ZoomLimits></tt:PTZConfiguration></trt:Profiles><trt:Profiles fixed=\"true\" token=\"profile_token_2\"><tt:Name>profile2</tt:Name><tt:VideoSourceConfiguration token=\"VideoSource_token_1\"><tt:Name>VideoSource_1</tt:Name><tt:UseCount>4</tt:UseCount><tt:SourceToken>VideoSource_token_1</tt:SourceToken><tt:Bounds height=\"1080\" width=\"1920\" y=\"0\" x=\"0\"></tt:Bounds></tt:VideoSourceConfiguration><tt:AudioSourceConfiguration token=\"AudioSource_token_1\"><tt:Name>AudioSource_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:SourceToken>AudioSource_token_1</tt:SourceToken></tt:AudioSourceConfiguration><tt:VideoEncoderConfiguration token=\"VideoEncode_token_2\"><tt:Name>VideoEncode_2</tt:Name><tt:UseCount>1</tt:UseCount><tt:Encoding>H264</tt:Encoding><tt:Resolution><tt:Width>704</tt:Width><tt:Height>576</tt:Height></tt:Resolution><tt:Quality>5</tt:Quality><tt:RateControl><tt:FrameRateLimit>25</tt:FrameRateLimit><tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>768</tt:BitrateLimit></tt:RateControl><tt:H264><tt:GovLength>100</tt:GovLength><tt:H264Profile>High</tt:H264Profile></tt:H264><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.1</tt:IPv4Address></tt:Address><tt:Port>51554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:VideoEncoderConfiguration><tt:AudioEncoderConfiguration token=\"AudioEncode_token_1\"><tt:Name>AudioEncode_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:Encoding>G711</tt:Encoding><tt:Bitrate>128</tt:Bitrate><tt:SampleRate>8</tt:SampleRate><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.3</tt:IPv4Address></tt:Address><tt:Port>53554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:AudioEncoderConfiguration><tt:VideoAnalyticsConfiguration token=\"VideoAnalytics0\"><tt:Name>MotionAnalytics_0</tt:Name><tt:UseCount>4</tt:UseCount><tt:AnalyticsEngineConfiguration><tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"CellMotionModule\"><tt:Parameters><tt:SimpleItem Value=\"42\" Name=\"Sensitivity\"></tt:SimpleItem><tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.002841\" y=\"0.003472\"/></tt:Transformation></tt:CellLayout></tt:ElementItem></tt:Parameters></tt:AnalyticsModule></tt:AnalyticsEngineConfiguration><tt:RuleEngineConfiguration><tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MotionDetectorRule\"><tt:Parameters><tt:SimpleItem Value=\"1\" Name=\"MinCount\"></tt:SimpleItem><tt:SimpleItem Value=\"0\" Name=\"AlarmOnDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"20000\" Name=\"AlarmOffDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"0P8A8A==\" Name=\"ActiveCells\"></tt:SimpleItem></tt:Parameters></tt:Rule></tt:RuleEngineConfiguration></tt:VideoAnalyticsConfiguration><tt:PTZConfiguration token=\"PTZToken\"><tt:Name>PTZconfig</tt:Name><tt:UseCount>3</tt:UseCount><tt:NodeToken>PTZToken</tt:NodeToken><tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace><tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace><tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace><tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace><tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace><tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace><tt:DefaultPTZSpeed><tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"0.100000001\" x=\"0.100000001\"></tt:PanTilt><tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom></tt:DefaultPTZSpeed><tt:DefaultPTZTimeout>PT10S</tt:DefaultPTZTimeout><tt:PanTiltLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:Range></tt:PanTiltLimits><tt:ZoomLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:Range></tt:ZoomLimits></tt:PTZConfiguration></trt:Profiles><trt:Profiles fixed=\"true\" token=\"profile_token_3\"><tt:Name>profile3</tt:Name><tt:VideoSourceConfiguration token=\"VideoSource_token_1\"><tt:Name>VideoSource_1</tt:Name><tt:UseCount>4</tt:UseCount><tt:SourceToken>VideoSource_token_1</tt:SourceToken><tt:Bounds height=\"1080\" width=\"1920\" y=\"0\" x=\"0\"></tt:Bounds></tt:VideoSourceConfiguration><tt:AudioSourceConfiguration token=\"AudioSource_token_1\"><tt:Name>AudioSource_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:SourceToken>AudioSource_token_1</tt:SourceToken></tt:AudioSourceConfiguration><tt:VideoEncoderConfiguration token=\"VideoEncode_token_3\"><tt:Name>VideoEncode_3</tt:Name><tt:UseCount>1</tt:UseCount><tt:Encoding>H264</tt:Encoding><tt:Resolution><tt:Width>352</tt:Width><tt:Height>288</tt:Height></tt:Resolution><tt:Quality>4</tt:Quality><tt:RateControl><tt:FrameRateLimit>25</tt:FrameRateLimit><tt:EncodingInterval>1</tt:EncodingInterval><tt:BitrateLimit>512</tt:BitrateLimit></tt:RateControl><tt:H264><tt:GovLength>100</tt:GovLength><tt:H264Profile>High</tt:H264Profile></tt:H264><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.2</tt:IPv4Address></tt:Address><tt:Port>52554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:VideoEncoderConfiguration><tt:AudioEncoderConfiguration token=\"AudioEncode_token_1\"><tt:Name>AudioEncode_1</tt:Name><tt:UseCount>3</tt:UseCount><tt:Encoding>G711</tt:Encoding><tt:Bitrate>128</tt:Bitrate><tt:SampleRate>8</tt:SampleRate><tt:Multicast><tt:Address><tt:Type>IPv4</tt:Type><tt:IPv4Address>239.0.0.3</tt:IPv4Address></tt:Address><tt:Port>53554</tt:Port><tt:TTL>1</tt:TTL><tt:AutoStart>false</tt:AutoStart></tt:Multicast><tt:SessionTimeout>PT60S</tt:SessionTimeout></tt:AudioEncoderConfiguration><tt:VideoAnalyticsConfiguration token=\"VideoAnalytics0\"><tt:Name>MotionAnalytics_0</tt:Name><tt:UseCount>4</tt:UseCount><tt:AnalyticsEngineConfiguration><tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"CellMotionModule\"><tt:Parameters><tt:SimpleItem Value=\"42\" Name=\"Sensitivity\"></tt:SimpleItem><tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.005682\" y=\"0.006944\"/></tt:Transformation></tt:CellLayout></tt:ElementItem></tt:Parameters></tt:AnalyticsModule></tt:AnalyticsEngineConfiguration><tt:RuleEngineConfiguration><tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MotionDetectorRule\"><tt:Parameters><tt:SimpleItem Value=\"1\" Name=\"MinCount\"></tt:SimpleItem><tt:SimpleItem Value=\"0\" Name=\"AlarmOnDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"20000\" Name=\"AlarmOffDelay\"></tt:SimpleItem><tt:SimpleItem Value=\"0P8A8A==\" Name=\"ActiveCells\"></tt:SimpleItem></tt:Parameters></tt:Rule></tt:RuleEngineConfiguration></tt:VideoAnalyticsConfiguration><tt:PTZConfiguration token=\"PTZToken\"><tt:Name>PTZconfig</tt:Name><tt:UseCount>3</tt:UseCount><tt:NodeToken>PTZToken</tt:NodeToken><tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace><tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace><tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace><tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace><tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace><tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace><tt:DefaultPTZSpeed><tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"0.100000001\" x=\"0.100000001\"></tt:PanTilt><tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom></tt:DefaultPTZSpeed><tt:DefaultPTZTimeout>PT10S</tt:DefaultPTZTimeout><tt:PanTiltLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:Range></tt:PanTiltLimits><tt:ZoomLimits><tt:Range><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>0</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:Range></tt:ZoomLimits></tt:PTZConfiguration></trt:Profiles></trt:GetProfilesResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";
//	xml_label(xmlstr, xml_label_out, NULL);
	//exit(1);

	ret = Onvif_GetDeviceInformation(url, username, passowrd, &info);
	if (ret != 200)
	{
		printf("onvif GetDeviceInformation err:%d\n", ret);
		return ret;
	}
	printf("Ma:%s, Mo:%s, F:%s,S:%s,HID:%s\n", info.Manufacturer, info.Model,
				info.FirmwareVersion, info.SerialNumber, info.HardwareId);

	Onvif_GetCapabilities_Media(url, username, passowrd, mediaUrl);

	printf("mediaUrl:%s\n", mediaUrl);

	struct OnvifDeviceProfiles profiles[5];
	memset(profiles, 0, sizeof(profiles));
	ret = Onvif_MediaService_GetProfiles(mediaUrl, username, passowrd,	profiles);
	if (ret != 200)
	{
		printf("onvif GetProfiles err:%d\n", ret);
		return ret;
	}

	char uri[300];

	for (int i = 0; i < 5; i++)
	{
		if (strlen(profiles[i].token) == 0)
			break;
		printf("token=%s,encode:%s,%dx%d\n", profiles[i].token,
					profiles[i].encodname, profiles[i].height, profiles[i].width);
		memset(uri, 0, sizeof(uri));
		ret = Onvif_MediaServer_GetStreamUri(mediaUrl, username, passowrd,
					profiles[i].token, uri);
		if (ret != 200)
		{
			printf("onvif GetStreamUri err:%d, token:%s\n", ret,
						profiles[i].token);
			continue;
		}
		OnvifURI_Decode(uri, uri, 300);
		printf("uri:%s\n", uri);
	}
	return ret;
}

#endif
