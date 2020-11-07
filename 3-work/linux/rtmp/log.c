/*
 * log.c
 *
 *  Created on: 2019年11月6日
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
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "log.h"

void log_printf(const char *levelstr, const char *module_name, const char *file,
		const char *function, int line, const char *format, ...)
{
	va_list v;
	char tmstr[22];

	if(strcmp(module_name, "WebVideo")==0)
		return ;

	struct timeval tp;
	gettimeofday(&tp, NULL);
	struct tm tm;
	localtime_r(&tp.tv_sec, &tm);

	strftime(tmstr, sizeof(tmstr) - 1, "%F %H:%M:%S", &tm);

	va_start(v, format);
	fprintf(stdout, "%s,%s|%s: %s,%s:%d:", tmstr, levelstr, module_name, file,
			function, line);
	vfprintf(stdout, format, v);
	va_end(v);
}
