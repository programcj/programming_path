/*
 * openlog_test.c
 *
 *  Created on: 2019年4月16日
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <assert.h>
#include <unistd.h>

int main(int argc, char **argv) {
	setlogmask(LOG_UPTO(LOG_DEBUG));

	openlog("Test", LOG_CONS | LOG_PID, LOG_USER );
	syslog(LOG_INFO,"This is a massage just for test");
	closelog();

	system("cat /etc/rsyslog.conf");
	system("cat /var/log/syslog");

	printf("PID:%d\n", getpid());
	return 0;
}
