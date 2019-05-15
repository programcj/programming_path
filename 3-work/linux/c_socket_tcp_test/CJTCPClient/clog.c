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

#include <pthread.h>
#include "clog.h"

static pthread_mutex_t mutex_x = PTHREAD_MUTEX_INITIALIZER;
//static int _logfd = 1;

static void _log_printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
}

int log_setconsole(int fd) {
	return 0;
}

int log_printf(LOG_LEVEL _level, const char *file, const char *function,
		int line, const char *format, ...) {
	int len = 0;
	char timeStr[50];
	time_t timep;
	struct tm *p;

	va_list list;

	if (strrchr(file, '/') != NULL)
		file = strrchr(file, '/');
	if (*file == '/')
		file++;

	time(&timep);
	p = localtime(&timep); /*取得当地时间*/

	sprintf(timeStr, "%d-%d-%d %02d:%02d:%02d", 1900 + p->tm_year,
			1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	va_start(list, format);

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
	vprintf(format, list);
	printf("\n");
	va_end(list);
	fflush(stdout);
	return len;
}

int log_printf_hex(LOG_LEVEL _level, const char *file, const char *function,
		int line, const void *data, int dlen, const char *format, ...) {
	/////////////////////////
	///int log_printf_hex(LOG_LEVEL _level, const char *file, const char *function, int line, const char *title,const char *format, const unsigned char *data, int dlen,...) {
	/////////////////////////
	int len = 0;
	char timeStr[50];
	time_t timep;
	struct tm *p;
	va_list list;

	if (strrchr(file, '/') != NULL)
		file = strrchr(file, '/');
	if (*file == '/')
		file++;

	time(&timep);
	p = localtime(&timep); /*取得当地时间*/

	sprintf(timeStr, "%d-%d-%d %02d:%02d:%02d", 1900 + p->tm_year,
			1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	pthread_mutex_lock(&mutex_x);
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
		printf("%02X ", ((unsigned char*)data)[len]);
	}
	printf("\n");
	fflush(stdout);
	pthread_mutex_unlock(&mutex_x);
	return len;
}
