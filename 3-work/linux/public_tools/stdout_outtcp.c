/*
 * logOutToTcp.c
 * 将日志输出重定向到网络TCP中
 *  Created on: 20190430
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

static volatile int _flagRun = 0;
static char _ipstr[20];
static int _port = 0;

void tools_stripv4cpy(char *dsc, const char *src)
{
	while (dsc && src && *src)
	{
		if ((*src < '0' || *src > '9') && *src != '.')
			break;
		*dsc++ = *src++;
	}
	if (dsc)
		*dsc = 0;
}

static int socket_nonblock(int fd, int flag) {
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

static int loop()
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = inet_addr(_ipstr);

	signal(SIGPIPE, SIG_IGN);

	if (sock <= 0)
	{
		return 0;
	}

	printf("start connect:%s:%d\n", _ipstr, _port);
	if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr))
			< 0)
	{
		fprintf(stderr, "connect err:%s:%d\n", _ipstr, _port);
		return 0;
	}

#ifdef SO_NOSIGPIPE
	int flag = 1;
	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &flag, sizeof flag);
#endif

	char buff[10];
	int ret = 0;

	int fdstdout = dup(STDOUT_FILENO);
	int fdstderr = dup(STDERR_FILENO);

	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	printf("connect success:%s:%d\n", _ipstr, _port);
	dup2(sock, STDOUT_FILENO);
	dup2(sock, STDERR_FILENO);

	while (1)
	{
		ret = recv(sock, buff, sizeof(buff), 0);
		if (ret == 0)
			break;

		if (ret < 0)
		{
			if (errno == EAGAIN ||errno == EINTR || errno == EWOULDBLOCK )
				continue;
			break;
		}
	}

	shutdown(sock, SHUT_RDWR);
	close(sock);

	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	dup2(fdstdout, STDOUT_FILENO);
	dup2(fdstderr, STDERR_FILENO);
	close(fdstdout);
	close(fdstderr);
	return 0;
}

static void *_run(void *arg)
{
	pthread_detach(pthread_self());
	loop();
	_flagRun = 0;
	return 0;
}

int stdout_outtcp_start(const char *ipv4str, unsigned short port)
{
	pthread_t pt;
	const char *ptr = NULL;
	unsigned short port = 0;
	if (_flagRun)
		return -1;

	_flagRun = 1;

	if ((ptr = strchr(ipv4str, ':')))
	{
		ptr++;
		port = atoi(ptr);
	}

	if (0 == port)
		port = 8089;

	_port = port;
	tools_stripv4cpy(_ipstr, ipv4str);

	pthread_create(&pt, NULL, _run, NULL);
	return 0;
}

void stdout_outtcp_stop()
{

}