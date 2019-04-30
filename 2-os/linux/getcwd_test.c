/*
 * getcwd_test.c
 *
 *  Created on: 2019年4月26日
 *      Author: cc
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
	const char *ldPath = getenv("LD_LIBRARY_PATH");
	char *tmpbuff = NULL;
	if (!ldPath)
		ldPath = "";

	tmpbuff = (char *) calloc(strlen(ldPath) + 150, 1);
	if (tmpbuff) {
		if (strlen(ldPath) > 0) {
			strcpy(tmpbuff, ldPath);
			strcat(tmpbuff, ":");
		}

		getcwd(tmpbuff + strlen(tmpbuff), 100);
		strcat(tmpbuff, "/");
		strcat(tmpbuff, "/libs");

		setenv("LD_LIBRARY_PATH", tmpbuff, 1);
		printf("LD_LIBRARY_PATH:[%s]\n", getenv("LD_LIBRARY_PATH"));

		free(tmpbuff);
	}
	return 0;
}
