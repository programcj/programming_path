/*
 * http.c
 *
 *  Created on: 2019年11月25日
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
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/poll.h>

int socket_readline(int fd, char *line, int maxsize) {
	char v;
	int ret;
	int count = 0;
	while (count < maxsize) {
		ret = recv(fd, &v, 1, 0);
		if (ret == 0) {
			return ret;
		}
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return ret;
		}
		*line++ = v;
		count++;
		if (v == '\n' && count < maxsize) {
			*line = 0;
			break;
		}
	}
	return count;
}

int http_post(const char *url, const char *authorization,
		const char *poststr, char **arg_http_resp, int *arg_resplen) {
	char ipstr[20] = "";
	int port = 80;
	char *path = NULL;
	int ret;

	*arg_http_resp = NULL;
	*arg_resplen = 0;
	{
		char *ptr;
		char *tmpp;
		int isssl = 0;

		if (strncmp(url, "http://", 7) == 0) {
			isssl = 0;
			ptr = (char*) url + 7;
		} else if (strncmp(url, "https://", 8) == 0) {
			isssl = 1;
			ptr = (char*) url + 8;
		}
		tmpp = ipstr;

		while (*ptr && tmpp != ipstr + 20) {
			if (*ptr != '.' && !isdigit(*ptr))
				break;
			*tmpp++ = *ptr++;
		}
		if (*ptr == ':') {
			port = atoi(ptr + 1);
			while (*ptr && *ptr != '/')
				ptr++;
		}

		path = ptr;
		if (!path || *path == 0)
			path = "/";
	}

	printf("ip:%s,port:%d, path:%s\n", ipstr, port, path);

	int fd;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port); //addr.sin_addr.s_addr=
	ret = inet_aton(ipstr, &addr.sin_addr);

	char buffcache[1024];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	ret = connect(fd, (struct sockaddr*) &addr, sizeof(addr));

	if (ret) {
		printf("connect err:%d,%s\n", errno, strerror(errno));
		goto _hquit;
	}

	sprintf(buffcache, "POST %s HTTP/1.1\r\n", path);
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "HOST:%s\r\n", ipstr);
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "Accept: */*\r\n");
	_socket_writestr(fd, buffcache);

	sprintf(buffcache, "Content-Type: application/soap+xml; charset=utf-8\r\n");
	_socket_writestr(fd, buffcache);

	if (authorization && strlen(authorization) > 0) {
		_socket_writestr(fd, authorization);
		_socket_writestr(fd, "\r\n");
	}

	sprintf(buffcache, "Content-Length: %ld\r\n", strlen(poststr));
	_socket_writestr(fd, buffcache);

	_socket_writestr(fd, "\r\n");
	_socket_writestr(fd, poststr);

	struct pollfd pos;
	pos.fd = fd;
	pos.events = POLLIN | POLLHUP | POLLERR | POLLNVAL;

	int content_length = -1;
	int http_head_readflag = 0;

	_socket_nonblock(fd, 1); //非阻塞

	FILE *fptmp = tmpfile();
	long int filesize = 0;
	long int http_head_size = 0;
	int loopflag = 1;
	while (loopflag) {
		if (filesize > 1024 * 1024 * 2) {
			perror("不能超过2M\n");
			break;
		}
		ret = poll(&pos, 1, 2 * 1000);
		if (ret <= 0) {
			printf("timeout ret=%d\n", ret);
			break;
		}

		if (pos.revents != POLLIN)
			continue;

		//read http head
		if (0 == http_head_readflag) {
			while (!http_head_readflag) {
				ret = socket_readline(fd, buffcache, sizeof(buffcache));
				if (ret <= 0) {
					loopflag = 0;
					break;
				}
				fprintf(fptmp, "%s", buffcache);
				//fputs(buffcache,fptmp);
				filesize = ftell(fptmp);

				if (strncasecmp(buffcache, "Content-Length:",
						strlen("Content-Length:")) == 0) {
					sscanf(buffcache, "%*[^0-9]%d", &content_length);
				}

				if ((buffcache[0] == '\r' && buffcache[1] == '\n')
						|| buffcache[0] == '\n') {
					http_head_readflag = 1;
					break;
				}
			}
			http_head_size = filesize;
			if (content_length == 0)
				break;
			continue;
		}

		{
			//memset(buffcache,0,sizeof(buffcache));
			ret = recv(fd, buffcache, sizeof(buffcache), 0);
			if (ret == 0) {
				printf("ret=0\n");
				break;
			}
			if (ret < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;
				break;
			}

			fwrite(buffcache, 1, ret, fptmp);
			filesize = ftell(fptmp);

			if (content_length >= 0) {
				content_length -= ret;
				if (content_length <= 0)
					break;
			}
		}
	}

	_hquit: shutdown(fd, SHUT_RDWR);
	close(fd);

	printf("http size:%ld\n", filesize);

	long httpcode = 0;

	if (filesize > 0) {
		rewind(fptmp);
		fscanf(fptmp, "%*9s %ld", &httpcode);
		ret = httpcode;

		if (httpcode == 200) { //去掉http头
			fseek(fptmp, http_head_size, SEEK_SET);

			filesize -= http_head_size;
			*arg_http_resp = (char*) malloc(filesize + 1); //\0
			(*arg_http_resp)[filesize] = 0;
			*arg_resplen = filesize;

			fread(*arg_http_resp, 1, filesize, fptmp);
		} else {
			*arg_http_resp = (char*) malloc(filesize + 1); //\0
			(*arg_http_resp)[filesize] = 0;
			*arg_resplen = filesize;
			fread(*arg_http_resp, 1, filesize, fptmp);
		}
	}
	fclose(fptmp);
	return ret;
}
