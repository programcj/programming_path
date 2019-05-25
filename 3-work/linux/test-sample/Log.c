/*
 * Log.c
 *
 *  Created on: 20190521
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "Log.h"

#define FIFO_OUT_NAME "/tmp/fifoUPService"

static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *_fpout = NULL;

static void fifoCheck()
{
	pthread_mutex_lock(&_lock);

	if (access(FIFO_OUT_NAME, F_OK) == 0)
	{
		if (_fpout == NULL)
		{
			int pipefd = open(FIFO_OUT_NAME, O_WRONLY | O_NONBLOCK);
			if (pipefd != -1)
			{
				_fpout = fdopen(pipefd, "w");
				printf("open fifo %s <%d> _fpout:%p.\n", FIFO_OUT_NAME, pipefd,
						_fpout);
			}
		}
	}
	else
	{
		if (_fpout != NULL)
		{
			fclose(_fpout);
			_fpout = NULL;
		}
	}
	//fileno(fp);
	pthread_mutex_unlock(&_lock);

}

static int _getCurrTime(char timeStr[30])
{
	struct tm t_tm;
	struct timeval tp;

	gettimeofday(&tp, NULL);
	localtime_r(&tp.tv_sec, &t_tm);

	snprintf(timeStr, 30, "[%4d/%02d/%02d %02d:%02d:%02d:%ld]",
			t_tm.tm_year + 1900, t_tm.tm_mon + 1, t_tm.tm_mday, t_tm.tm_hour,
			t_tm.tm_min, t_tm.tm_sec, tp.tv_usec / 100);
	return 0;
}

void LogPrint(int level, const char *file, const char *fun, int line,
		const char *format, ...)
{
	char timestr[33];
	va_list v;

	fifoCheck();

	_getCurrTime(timestr);

	va_start(v, format);

	if (_fpout != NULL)
	{
		pthread_mutex_lock(&_lock);
		fprintf(_fpout, "%s %c:%s,%s[%d]", timestr, level, file, fun, line);
		vfprintf(_fpout, format, v);
		fflush(_fpout);
		pthread_mutex_unlock(&_lock);
	}
	else
	{
		fprintf(stdout, "%s %c:%s,%s[%d]", timestr, level, file, fun, line);
		vfprintf(stdout, format, v);
	}

	va_end(v);
}

#ifdef LOG_SHOW_EXE

int main(const char *argc, const char **argv)
{
	uint8_t buff[1024];
	int ret;

	if(access(FIFO_OUT_NAME, F_OK)!=0)
	{
		ret = mkfifo(FIFO_OUT_NAME, 0777);

		if (ret != 0)
		{
			perror("mkfifo..");
			exit(1);
		}
		printf("open fifo >%s\n", FIFO_OUT_NAME);
	}
	//unlink("/tmp/fifoUPService");

	int fd = open(FIFO_OUT_NAME, O_RDONLY | O_NONBLOCK);
	if (fd != -1)
	{
		int rlen = 0;
		while (1)
		{
			memset(buff, 0, sizeof(buff));
			rlen = read(fd, buff, sizeof(buff));
			if (rlen <= 0)
			{
				if (errno == EAGAIN)
				continue;
				if (errno == 0)
				continue;

				perror("...");
				break;
			}
			printf("%s", buff);
			fsync(fd);
		}
		close(fd);

		unlink(FIFO_OUT_NAME);
	}
	return 0;
}

#endif

