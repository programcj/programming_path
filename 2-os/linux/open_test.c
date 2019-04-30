/*
 * open_test.c
 *
 *  Created on: 2019年4月15日
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <printf.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static int file_lock(int fd) {
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}

void test_1() {
	printf("-begin-start-%s\n", __FUNCTION__);
	int fd = 0;
	char *filename = "/tmp/aaaaaa";
	char buf[100];

	fd = open(filename, O_RDWR | O_CREAT,
			(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

	if (file_lock(fd) == 0) {
		ftruncate(fd, 0);
		sprintf(buf, "%ld\n", (long) getpid());
		write(fd, buf, strlen(buf) + 1);
	}

	close(fd);

	printf("-begin-end-%s\n", __FUNCTION__);
}

int main(int argc, char **argv) {
	test_1();

	return 0;
}
