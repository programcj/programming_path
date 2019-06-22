/*
 * time_test.c
 *
 *  Created on: 2019年4月17日
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void time_test_0() {
	printf("-begin-start-%s\n", __FUNCTION__);
	struct tm t_tm;
	time_t timer;
	char timeStr[50];

	timer = time(NULL);
	localtime_r(&timer, &t_tm);

	sprintf(timeStr, "%4d/%02d/%02d %02d:%02d:%02d", t_tm.tm_year + 1900,
			t_tm.tm_mon + 1, t_tm.tm_mday, t_tm.tm_hour, t_tm.tm_min,
			t_tm.tm_sec);

	printf("-end-start-%s\n", __FUNCTION__);
}

void time_test_1() {
	printf("-begin-start-%s\n", __FUNCTION__);

	printf("-end-start-%s\n", __FUNCTION__);
}

int main(int argc, char **argv) {
	time_test_0();
	time_test_1();
	return 0;
}
