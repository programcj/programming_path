/*
 * tmpfile_test.c
 *
 *  Created on: 2019年7月11日
 *      Author: cc
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

int main(int argc, char **argv) {
	FILE *fp = NULL;
	char tmpnamestr[100]="/tmp/aaaa.XXXXXX";

	//tmpfile 原理
	int fd=mkstemp(tmpnamestr);
	unlink(tmpnamestr);//
	close(fd);

	printf("tmpnamestr=%s\n", tmpnamestr);
	//printf("tmpname=%s\n", tmpnam_r(tmpnamestr));

	fp=tmpfile();
	if (fp) {
		printf("tmpfile %p\n", fp);
		fprintf(fp, "hello tmpfile!");
		sleep(10);
		fclose(fp);
	}
	return 0;
}
