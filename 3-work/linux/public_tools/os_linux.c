/*
 * os_linux.c
 *
 *  Created on: 2017年9月15日
 *      Author: cj
 *      Email: 593184917@qq.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include "tools.h"

void os_linux_drop_caches(int value) {
	char buf[2];
	switch (value) {
	case 1:
		strcpy(buf, "1");
		break;
	case 2:
		strcpy(buf, "2");
		break;
	case 3:
	default:
		strcpy(buf, "3");
		break;
	}
	tools_write_file("/proc/sys/vm/drop_caches", buf); //清理Linux缓存
}

int os_proc_meminfo_read(const char *name, unsigned long *value) {
#ifdef SYSINFO_READ
	struct sysinfo info;
	memset(&info, 0, sizeof info);
	sysinfo(&info);

#else
	char buff[100];
	char *head = NULL;
	char *tail;
	int ret = -1;

	FILE *fp = fopen("/proc/meminfo", "r");
	if (fp == NULL)
		return -1;

	while (!feof(fp)) {
		fgets(buff, 100, fp);
		head = strchr(buff, ':');
		if (head == NULL)
			continue;
		*head = '\0';
		head += 1;
		if (strcmp(buff, name) == 0) {
			ret = 0;
			*value = strtoull(head, &tail, 10);
		}
	}
	fclose(fp);
#endif
	return ret;
}

int os_linux_callcmd(int *rfd, int *wfd, char *cmdargs[]) {
	pid_t pid = 0;
	int pipe_r[2];
	int pipe_w[2];
	int ret = 0;

	ret = pipe(pipe_r);
	ret = pipe(pipe_w);

	*rfd = pipe_r[0];
	*wfd = pipe_w[1];

	pid = fork();
	if (pid < 0)
		return -1;

	if (pid == 0) {
		close(pipe_w[1]);
		dup2(pipe_w[0], STDIN_FILENO);
		close(pipe_w[0]);

		close(pipe_r[0]);
		dup2(pipe_r[1], STDOUT_FILENO);
		close(pipe_r[1]);

		execvp(cmdargs[0], cmdargs);
		return 0;
	}
	usleep(100);
	close(pipe_w[0]);
	close(pipe_r[1]);
	//
	return pid;
}

int io_nonblock(int fd, int flag) {
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return -1;
	if (flag)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		return -1;
	return 0;
}
