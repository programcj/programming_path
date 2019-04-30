/*
 * main.c
 *
 *  Created on: 2018年4月23日
 *      Author: cj
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mem.h"

#include "../public_tools/clog.h"

int _logback(LOG_LEVEL level, const char *mode, const char *file,
		const char *fun, int line, const char *fmt, va_list ap){
	printf("%d,%s,%c:%s,%s,%d:",(int)time(NULL), mode,level, file, fun, line);
	vprintf(fmt,ap);
	return 1;
}

int main(int argc, char **argv) {
	void *ptr = NULL;
	int i = 0;

	openlog("mode", LOG_CONS | LOG_PID, LOG_DAEMON);
	syslog(LOG_DEBUG, "...");

	log_setbackfun(_logback);
	printf("start ...\n");

	for (i = 0; i < 10; i++) {
		ptr = malloc(200);

		log_d("malloc\n");

		logs_d("aff %d", i);
		mem_debug();

		free(ptr);

		log_d("free\n");
		mem_debug();
	}

	closelog();
	return 0;
}
