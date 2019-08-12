/*
 * rtsp_service.c
 *
 *  Created on: 2019年6月16日
 *      Author: cc
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "proto_service.h"
#include "cache_array.h"
#include "../socket_opt/socket_opt.h"

struct rtsp_session {
	struct proto_session *tcpsession;
	struct cache_array in_array;
	struct cache_array out_array;
	int cseq;
};

//#define CONFIG_SESSION_RWBUFF_SIZE	(1024 * 20)
#define CONFIG_SESSION_RWBUFF_SIZE	1024

struct rtsp_session *rtsp_session_new() {
	struct rtsp_session *session = (struct rtsp_session *) calloc(1, sizeof(struct rtsp_session));
	if (session) {
		cache_array_init(&session->in_array, CONFIG_SESSION_RWBUFF_SIZE);
		cache_array_init(&session->out_array, CONFIG_SESSION_RWBUFF_SIZE);
	}
	return session;
}

void rtsp_session_delete(struct rtsp_session *session) {
	if (session) {
		cache_array_destory(&session->in_array);
		cache_array_destory(&session->out_array);
		free(session);
	}
}

char *strhttp_head_end(char *strhead, int length) {
	char *tmptr = NULL, *ctrlf = NULL, *pheadend = NULL;
	int end = length;
	tmptr = strhead;
	//int isend = 0;
	while (tmptr < strhead + end) {
		if (*tmptr == '\n') {
			if (tmptr - ctrlf <= 2) {
				ctrlf = tmptr;
				//isend = 1;
				//break;
				return ctrlf + 1;
			}
			ctrlf = tmptr;
		}
		tmptr++;
	}
	return NULL;
}

int strhttp_head_getvalueint(const char *httphead, const char *httpend, const char *name,
		int defvalue) {
	const char *item = httphead;
	const char *lineend = NULL;
	int value;

	//去掉首行
	while (item < httpend) {
		lineend = item;
		while (lineend < httpend) {
			if (*lineend++ == '\n')
				break;
		}

		if (lineend < httpend) {
			if (strncasecmp(item, name, strlen(name)) == 0) {
				//去掉:和' '
				item += strlen(name);
				while (*item == ':' || *item == ' ')
					item++;
				value = atoi(item);
				return value;
			}
		}
		item = lineend;
	}
	return defvalue;
}

int strhttp_head_getvaluestr(const char *httphead, const char *httpend, const char *name,
		char *value, int len) {
	const char *item = httphead;
	const char *lineend = NULL;

	//去掉首行
	while (item < httpend) {
		lineend = item;
		while (lineend < httpend) {
			if (*lineend++ == '\n')
				break;
		}

		if (lineend < httpend) {
			if (strncasecmp(item, name, strlen(name)) == 0) {
				//去掉:和' '
				item += strlen(name);
				while (*item == ':' || *item == ' ')
					item++;
				while (item < httpend && *item != '\r' && *item != '\n' && len > 0) {
					*value++ = *item++;
					len--;
				}
				return 0;
			}
		}
		item = lineend;
	}
	return -1;
}

void OPTIONS_handle(struct rtsp_session *rtspsession, const char *strheaditem,
		const char *strheadend) {
	char *strout = cache_array_remainptr(&rtspsession->out_array);
	const char *format =
			"RTSP/1.0 200 OK\r\nCSeq: %d\r\nServer: cjrtsp\r\nCache-Control: no-cache\r\n"
					"Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, OPTIONS, ANNOUNCE, RECORD, GET_PARAMETER\r\n\r\n";
	int len = 0;
	char user_agent[30];

	if (cache_array_remainlen(&rtspsession->out_array) < strlen(format) + 30) {
		cache_array_remainlen_tran(&rtspsession->out_array);
	}

	if (cache_array_remainlen(&rtspsession->out_array) < strlen(format) + 30) {
		fprintf(stderr, "out array err. remainlen:%d\n",
				cache_array_remainlen(&rtspsession->out_array));
		return;
	}

	strout = cache_array_remainptr(&rtspsession->out_array);
	len = sprintf(strout, format, rtspsession->cseq);

	printf("sprint len:%d\n", len);

	memset(user_agent, 0, sizeof(user_agent));
	strhttp_head_getvaluestr(strheaditem, strheadend, "User-Agent", user_agent,
			sizeof(user_agent) - 1);

	printf("cseq:%d\n", rtspsession->cseq);
	printf("user_agent:%s\n", user_agent);

	cache_array_uselen_add(&rtspsession->out_array, len);
}

//ffplay rtsp://127.0.0.1:8880/Demo.264
int socket_fd_printf(int fd, const char *format, ...) {
	int fdn = dup(fd);
	FILE *fp = fdopen(fdn, "w");
	va_list v;
	int ret = 0;
	va_start(v, format);
	ret = vfprintf(fp, format, v);
	va_end(v);
	fclose(fp);
	return ret;
}

int socket_fd_writestr(int fd, const char *str) {
	return send(fd, str, strlen(str), 0);
}

int write_str(int fd, const char *str) {
	return write(fd, str, strlen(str));
}

void DESCRIBE_handler(struct rtsp_session *rtspsession, const char *url, const char *strheaditem,
		const char *strheadend) {
	//ffmpeg -rtsp_transport tcp -i rtsp://184.72.239.149:554/vod/mp4://BigBuckBunny_175k.mov
//DESCRIBE rtsp://184.72.239.149:554/vod/mp4://BigBuckBunny_175k.mov RTSP/1.0
//Accept: application/sdp
//CSeq: 2
//User-Agent: Lavf56.40.101

	//Session DP Description
	char strtmp[10];
	int fd = rtspsession->tcpsession->fd;
	int content_len = 0;
	char *content_str = NULL;
	//内容长度如何计算?
	int fds[2];
	pipe(fds);

	write_str(fds[1], "v=0\r\n");
	write_str(fds[1], "o=- 1560679407630764 1 IN IP4 127.0.1.1\r\n");
	write_str(fds[1], "s=H.264 Video, streamed by the LIVE555 Media Server\r\n");
	write_str(fds[1], "i=Demo.264\r\n");
	write_str(fds[1], "t=0 0\r\n");
	write_str(fds[1], "a=tool:LIVE555 Streaming Media v2018.08.28\r\n");
	write_str(fds[1], "a=type:broadcast\r\n");
	write_str(fds[1], "a=control:*\r\n");
	write_str(fds[1], "a=range:npt=0-\r\n");
	write_str(fds[1], "a=x-qt-text-nam:H.264 Video, streamed by the LIVE555 Media Server\r\n");
	write_str(fds[1], "a=x-qt-text-inf:Demo.264\r\n");
	write_str(fds[1], "m=video 0 RTP/AVP 96\r\n");
	write_str(fds[1], "c=IN IP4 0.0.0.0\r\n");
	write_str(fds[1], "b=AS:500\r\n");
	write_str(fds[1], "a=rtpmap:96 H264/90000\r\n");
	write_str(fds[1],
			"a=fmtp:96 packetization-mode=1;profile-level-id=4D401E;sprop-parameter-sets=J01AHqkYMB73oA==,KM4C+IA=\r\n");
	write_str(fds[1], "a=control:track1\r\n");

	socket_get_count_in_queue(fds[0], &content_len);
	if (content_len > 0) {
		content_str = (char*) malloc(content_len);
		read(fds[0], content_str, content_len);
	}
	close(fds[0]);
	close(fds[1]);

	socket_fd_writestr(fd, "RTSP/1.0 200 OK\r\n");
	sprintf(strtmp, "CSeq:%d\r\n", rtspsession->cseq);
	socket_fd_writestr(fd, strtmp);
	socket_fd_writestr(fd, "Date: Sun, Jun 16 2019 10:03:27 GMT\r\n");
	socket_fd_writestr(fd, "Content-Base: ");
	socket_fd_writestr(fd, url);
	socket_fd_writestr(fd, "\r\n");
	socket_fd_writestr(fd, "Content-Type: application/sdp\r\n");
	sprintf(strtmp, "Content-Length: %d\r\n", content_len);
	socket_fd_writestr(fd, strtmp);
	socket_fd_writestr(fd, "\r\n");
	socket_fd_writestr(fd, content_str);
	if (content_str) {
		free(content_str);
	}
}

//25fps //
int rtsp_handle(struct rtsp_session *rtspsession) {
	char *strhttphead = (char*) cache_array_useptr(&rtspsession->in_array);
	char *strhttpend = strhttp_head_end(strhttphead, cache_array_uselen(&rtspsession->in_array));

	if (!strhttpend)
		return 0;

	if (strhttpend) {
		printf("http head ok ---- \n");
		for (char *ptr = cache_array_useptr(&rtspsession->in_array); ptr < strhttpend; ptr++) {
			if (*ptr == '\r')
				continue;
			printf("%c", *ptr);
		}
		printf("-----\n");
	}

	const char *next;
	char method[30]; //options
	char *ptr1, *ptr2;
	char *url;

	//next 为http下一行
	next = cache_array_useptr(&rtspsession->in_array);
	while (next < strhttpend) {
		if (*next++ == '\n')
			break;
	}
	ptr1 = method;
	ptr2 = strhttphead;
	while (ptr1 < &method[sizeof(method) - 1] && ptr2 < strhttpend) {
		if (*ptr2 == ' ')
			break;
		*ptr1++ = *ptr2++;
	}
	if (ptr2 >= strhttpend) {
		fprintf(stderr, "err proto...[%s]\n", strhttphead);
		return -1;
	}

	*ptr1 = 0;
	ptr2++;

	url = ptr2;
	while (*ptr2 != ' ')
		ptr2++;
	*ptr2 = 0;
	//
	rtspsession->cseq = strhttp_head_getvalueint(next, strhttpend, "cseq", -1);

	printf("method:%s URL:%s\n", method, url);

	if (strcasecmp(method, "OPTIONS") == 0) {
		OPTIONS_handle(rtspsession, next, strhttpend);
	}
	if (strcasecmp(method, "DESCRIBE") == 0) {
		DESCRIBE_handler(rtspsession, url, next, strhttpend);
	}
	cache_array_usedlen_sub(&rtspsession->in_array, strhttpend - strhttphead);
	return 0;
}

int rtsp_service_rwhandle(struct proto_service_context *context, struct proto_session *session,
		int event) {
	int ret = 0;
	int fd = session->fd;

	//printf("fd=%d event:%d\n", fd, event);

	switch (event) {
	case PS_EVENT_SESSION_CREATE: {
		struct rtsp_session *rtsp = (struct rtsp_session *) rtsp_session_new();
		rtsp->tcpsession = session;
		session->user = rtsp;
		printf("PS_EVENT_SESSION_CREATE fd=%d\n", fd);
	}
		break;
	case PS_EVENT_SESSION_DESTORY:
		rtsp_session_delete((struct rtsp_session *) session->user);
		printf("PS_EVENT_SESSION_DESTORY fd=%d\n", fd);
		break;
	case PS_EVENT_ACCEPT:
		printf("PS_EVENT_ACCEPT, fd=%d\n", fd);
		//socket_set_sendbuf_size(fd, 1024 * 1024); //1M
		//socket_setopt(fd, SO_SNDBUFFORCE, 1024 * 1024 * 2);
		break;
	case PS_EVENT_RECV: {
		struct rtsp_session *rtspsession = (struct rtsp_session *) session->user;

		if (cache_array_remainlen(&rtspsession->in_array) == 0) {
			printf("--------------------------------cache_array_remainlen_tran...\n");
			if (0 == cache_array_remainlen_tran(&rtspsession->in_array)) {
				fprintf(stderr, "recv buff over\n");
				return -1;
			}
		}

		ret = recv(fd, cache_array_remainptr(&rtspsession->in_array),
				cache_array_remainlen(&rtspsession->in_array), 0);
		if (ret == 0) {
			fprintf(stderr, "recv ret=0 fd=%d err=%d\n", fd, socket_get_error(fd));
			return -1;
		}
		if (ret == -1) {
			if (errno == EAGAIN || EINTR == errno)
				return 0;
			fprintf(stderr, "recv err fd=%d, e=%d\n", fd, errno);
			return -1;
		}

		cache_array_uselen_add(&rtspsession->in_array, ret);
		rtsp_handle(rtspsession);

		//使能写
		if (cache_array_uselen(&rtspsession->out_array))
			proto_service_on_write(context, rtspsession->tcpsession, 1);
		else
			proto_service_session_timeout_start(session, 3);
	}
		break;
	case PS_EVENT_WRITE: {
		struct rtsp_session *rtspsession = (struct rtsp_session *) session->user;

		if (cache_array_uselen(&rtspsession->out_array) == 0) {
			proto_service_on_write(context, rtspsession->tcpsession, 0);
			return 0;
		}
		ret = send(fd, cache_array_useptr(&rtspsession->out_array),
				cache_array_uselen(&rtspsession->out_array), 0);
		if (ret == 0) {
			fprintf(stderr, "recv ret=0 fd=%d err=%d\n", fd, socket_get_error(fd));
			return -1;
		}
		if (ret == -1) {
			if (errno == EAGAIN || EINTR == errno)
				return 0;
			fprintf(stderr, "recv err fd=%d, e=%d\n", fd, errno);
			return -1;
		}

		cache_array_usedlen_sub(&rtspsession->out_array, ret);
	}
		//proto_service_session_timeout_stop(session);
//		ret = send(fd, buff, strlen(buff), 0);
//		printf("PS_EVENT_WRITE ret=%d fd=%d,%s\n", ret, fd, buff);
//		//proto_service_on_write(context, session, 0);
//		if (proto_service_session_is_choke(session)) {
//			printf("choke-> fd=%d\n", session->fd);
//			proto_service_session_timeout_start(session, 3);
//		}
//
//		if (ret < strlen(buff)) {
//			printf("ret < strlen(buff)\n");
//		}
//		proto_service_on_write(context, session, 0);
//
//		if (ret == 0)
//			return -1;

		break;

	case PS_EVENT_CLOSE:
		printf("PS_EVENT_CLOSE fd=%d\n", fd);
		break;

	case PS_EVENT_TIMEROUT: {
		struct rtsp_session *rtspsession = (struct rtsp_session *) session->user;
		printf("PS_EVENT_TIMEROUT fd=%d\n", fd);
		printf("ret=%d , is choke:%d  uselen:%d\n", ret, proto_service_session_is_choke(session),
				cache_array_uselen(&rtspsession->in_array));
		proto_service_on_write(context, session, 1);
	}
		break;
	}
	return 0;
}

void rtsp_service_loop_sync() {

}

void rtsp_service_test() {
	char *buff =
			"OPTIONS rtsp://127.0.0.1:8880/Demo.264 RTSP/1.0\r\nCSeq: 1\r\nUser-Agent: Lavf56.40.101\r\n\r\n";

	struct rtsp_session *rtspsession = (struct rtsp_session *) rtsp_session_new();

	strcpy(cache_array_remainptr(&rtspsession->in_array), buff);
	cache_array_uselen_add(&rtspsession->in_array, strlen(buff));

	char *content_str = NULL;
	int content_len = 0;
	int fds[2];
	pipe(fds);

	write_str(fds[1], "v=0\r\n");
	write_str(fds[1], "o=- 1560679407630764 1 IN IP4 127.0.1.1\r\n");
	write_str(fds[1], "s=H.264 Video, streamed by the LIVE555 Media Server\r\n");
	write_str(fds[1], "i=Demo.264\r\n");
	write_str(fds[1], "t=0 0\r\n");
	write_str(fds[1], "a=tool:LIVE555 Streaming Media v2018.08.28\r\n");
	write_str(fds[1], "a=type:broadcast\r\n");
	write_str(fds[1], "a=control:*\r\n");
	write_str(fds[1], "a=range:npt=0-\r\n");
	write_str(fds[1], "a=x-qt-text-nam:H.264 Video, streamed by the LIVE555 Media Server\r\n");
	write_str(fds[1], "a=x-qt-text-inf:Demo.264\r\n");
	write_str(fds[1], "m=video 0 RTP/AVP 96\r\n");
	write_str(fds[1], "c=IN IP4 0.0.0.0\r\n");
	write_str(fds[1], "b=AS:500\r\n");
	write_str(fds[1], "a=rtpmap:96 H264/90000\r\n");
	write_str(fds[1],
			"a=fmtp:96 packetization-mode=1;profile-level-id=4D401E;sprop-parameter-sets=J01AHqkYMB73oA==,KM4C+IA=\r\n");
	write_str(fds[1], "a=control:track1\r\n");

	socket_get_count_in_queue(fds[0], &content_len);
	if (content_len > 0) {
		content_str = (char*) malloc(content_len);
		read(fds[0], content_str, content_len);
	}
	close(fds[0]);
	close(fds[1]);

	if (content_str) {
		printf("content_str=[%s]", content_str);
	}
	rtsp_handle(rtspsession);
	//exit(1);
}
