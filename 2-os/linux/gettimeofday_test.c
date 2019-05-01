/*
 * gettimeofday_test.c
 *
 *  Created on: 2019年4月28日
 *      Author: cc
 */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static long gettimeofday2millis(int usecnumber) {
	struct timeval tv;
	long v = 0;
	gettimeofday(&tv, NULL);
	if (usecnumber == 3) {
		v = tv.tv_sec * 1000;
		v += tv.tv_usec / 1000;
	}
	if (usecnumber == 4) {
		v = tv.tv_sec * 10000;
		v += tv.tv_usec / 100;
	}
	if (usecnumber == 5) {
		v = tv.tv_sec * 100000;
		v += tv.tv_usec / 10;
	}
	return v;
}

int main(int argc, char **argv) {
	struct timeval tv;
	long tims = 0;
	gettimeofday(&tv, NULL);
	printf("%d %d\n", tv.tv_sec, tv.tv_usec);
	printf("%ld\n", gettimeofday2millis(3));
	printf("%ld\n", gettimeofday2millis(4));
	printf("%ld\n", gettimeofday2millis(5));
	return 0;
}

