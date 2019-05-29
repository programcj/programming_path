/*
 * clog.c
 *
 *  Created on: 2017年3月25日
 *      Author: cj
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysctl.h>
#include "clog.h"

/**
 openlog("mode", LOG_CONS|LOG_PID,0);
 syslog(LOG_DEBUG, "...");
 closelog();
 */
static int _line_flag = 1;
static volatile int _log_level = L_ALL;
static log_backcallfun *_backcallfun = NULL;
static char _outfile[FILENAME_MAX];

void log_setout_file(const char *file) {
	strcpy(_outfile, file);
}

void log_setout_tcp(const char *ipv4str) {
	strcpy(_outfile, ipv4str);
}

void log_setbackfun(log_backcallfun *fun) {
	_backcallfun = fun;
}

void log_get_pthread_curname(char name[16]) {
	prctl(PR_GET_NAME, name);
}

static void _log_printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

extern int log_setlevel(int level) {
	_log_level = level;
	return 0;
}

static FILE *log_getout() {
	static FILE *outfp = NULL;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

	if (strlen(_outfile) == 0) {
		if (outfp != NULL) {
			fclose(outfp);
			outfp = NULL;
		}
		return NULL;
	}

	if (strncmp(_outfile, "tcp://", strlen("tcp://")) == 0) {
		//out tcp
		if (outfp == NULL) {
			int socket = socket(AF_INET, SOCK_STREAM, 0);
			char ipaddr[20];
			struct sockaddr_in taddr;
			if (socket == -1)
				return NULL;
			char *ptr, *pto;

			taddr.sin_family = AF_INET;
			inet_pton(AF_INET, &taddr.sin_addr, ipaddr);
		}
		return outfp;
	}

	if (access(_outfile, F_OK) == 0) {
		pthread_mutex_lock(&lock);
		if (outfp == NULL) {
			int fd = open(_outfile, O_NONBLOCK);
			if (fd > 0) {
				outfp = fdopen(fd, "w");
			}
		}
		pthread_mutex_unlock(&lock);
	} else {
		if (outfp) {
			pthread_mutex_lock(&lock);
			if (outfp)
				fclose(outfp);
			outfp = NULL;
			pthread_mutex_unlock(&lock);
		}
	}
}
return outfp;
}

static int log_vprintf(int level, const char *mode, const char *file,
	const char *fun, int line, const char *format, va_list ap) {
time_t timep;
struct tm p;
char timstr[20];
char pthread_curname[16];
FILE *outfp = NULL;

if (!mode) {
	log_get_pthread_curname(pthread_curname);
	mode = pthread_curname;
}

if (_backcallfun) {
	//LOG_LEVEL level, const char *mode, const char *file,const char *fun, int line, const char *fmt, va_list ap
	if (_backcallfun(level, mode, file, fun, line, format, ap))
		return 0;
}

time(&timep);
localtime_r(&timep, &p); //取得当地时间

if (strrchr(file, '/') != NULL) //只显示文件名，不显示路径
	file = strrchr(file, '/');
if (*file == '/')
	file++;

//时间
sprintf(timstr, "%d%02d%02d_%02d:%02d:%02d", 1900 + p.tm_year, 1 + p.tm_mon,
		p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);

outfp = log_getout();

if (!outfp)
	outfp = stdout;

if (_line_flag) {
	char loghs[99];
	char *ptr = loghs;
	int index = 0;

	if (index < sizeof(loghs))
		index += snprintf(loghs, sizeof(loghs), "%s", timstr);

	if (index < sizeof(loghs))
		index += snprintf(ptr + index, sizeof(loghs) - index,
				"|%s|%c:%s,%s,%d:", mode, level, file, fun, line);

	if (index < sizeof(loghs))
		index += vsnprintf(ptr + index, sizeof(loghs) - index, format, ap);
	fprintf(outfp, "%s", loghs);
	fflush(stdout);
} else {
	fprintf(outfp, "%s", timstr);
	fprintf(outfp, "|%s|%c:%s,%s,%d:", mode, level, file, fun, line);
	vfprintf(outfp, format, ap);
	fflush(outfp);
}
return 0;
}

int log_printf(int level, const char *modename, const char *file,
	const char *function, int line, const char *format, ...) {
va_list ap;
va_start(ap, format);
log_vprintf(level, modename, file, function, line, format, ap);
va_end(ap);
return 0;
}

int log_printf_hex(int _level, const char *file, const char *function, int line,
	const void *data, int dlen, const char *format, ...) {
int len = 0;
char timeStr[20];
time_t timep;
struct tm *p;
va_list list;

if ((_level & _log_level) == 0)
	return 0;

if (strrchr(file, '/') != NULL)
	file = strrchr(file, '/');
if (*file == '/')
	file++;

time(&timep);
p = localtime(&timep); /*取得当地时间*/

sprintf(timeStr, "%d-%d-%d %02d:%02d:%02d", 1900 + p->tm_year, 1 + p->tm_mon,
		p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

//pthread_mutex_lock(&mutex_x);
_log_printf("%s", timeStr);
switch (_level) {
case L_DEBUG:
	_log_printf("-DEBUG:%s,%s,%d:", file, function, line);
	break;
case L_INFO:
	_log_printf("-INFO:%s,%s,%d:", file, function, line);
	break;
case L_WRING:
	_log_printf("-WRING:%s,%s,%d:", file, function, line);
	break;
case L_ERR:
	_log_printf("-ERR:%s,%s,%d:", file, function, line);
	break;
default:
	break;
}

va_start(list, format);
vprintf(format, list);
va_end(list);

for (len = 0; len < dlen; len++) {
	printf("%02X ", ((unsigned char*) data)[len]);
}
printf("\n");
fflush(stdout);
//pthread_mutex_unlock(&mutex_x);
return len;
}

int log_printf2(unsigned char flag, int level, const char *file,
	const char *function, int line, const char *format, ...) {
int len = 0;
char timeStr[50];
time_t timep;
struct tm *p;

va_list ap;

if (!flag)
	return 0;

if ((level & _log_level) == 0)
	return 0;

va_start(ap, format);
log_vprintf(level, NULL, file, function, line, format, ap);
va_end(ap);
return len;
}

//#define CLOG_TO_FIFO_EXE

#ifdef CLOG_TO_FIFO_EXE

int main(int argc, char **argv) {

return 0;
}

#endif
