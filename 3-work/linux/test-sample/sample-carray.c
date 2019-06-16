/*
 ============================================================================
 Name        : test-c.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>

struct carray {
	unsigned char *data;
	int size;
	int point_end;
	int remain_len;
};

int carray_init(struct carray *array, int size) {
	memset(array, 0, sizeof(typeof(*array)));
	array->size = size;
	array->data = (unsigned char*) malloc(size);
	return 0;
}

static inline int carray_uselen(struct carray *array) {
	return array->point_end;
}

static inline int carray_remainlen(struct carray *array) {
	return array->size - array->point_end;
}

static inline unsigned char *carray_remainptr(struct carray *array) {
	return array->data + array->point_end;
}

static inline int carray_remaindata_append(struct carray *array, void *data, int size) {
	if (size + array->point_end > array->size)
		size = array->size - array->point_end;

	memcpy(array->data + array->point_end, data, size);
	array->point_end += size;
	return size;
}

static inline int carray_uselen_add(struct carray *array, int len) {
	array->point_end += len;
	return array->point_end;
}

static inline void carray_usedlen_sub(struct carray *array, int len) {
	if (len > array->point_end)
		len = array->point_end;
	memcpy(array->data, array->data + len, array->point_end - len);
	array->point_end = array->point_end - len;
}

static inline void *carray_useptr(struct carray *array) {
	return (void*)array->data;
}

int fds[2];

void *run(void *arg) {

	return NULL;
}

int main(void) {
	struct carray array;
	int ret = 0;

	pthread_t pt;
	pthread_create(&pt, NULL, run, NULL);

	/* 创建套接字对 */
	ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);

	carray_init(&array, 100);
	memset(carray_remainptr(&array), 'A', carray_remainlen(&array));
	char *buff =
			"OPTIONS rtsp://127.0.0.1:8880/Demo.264 RTSP/1.0\r\nCSeq: 1\r\nUser-Agent: Lavf56.40.101\r\n\r\n";
	send(fds[0], buff, strlen(buff), 0);

	printf("carray_remainlen(&array)=%d\n", carray_remainlen(&array));

	if (carray_remainlen(&array) == 0) {
		printf("cache memory over\n");
	}

	ret = recv(fds[1], carray_remainptr(&array), carray_remainlen(&array), 0);
	if (ret > 0) {
		carray_uselen_add(&array, ret);
	}

	if (ret == -1) {
		perror("recv err\n");
	}
	printf("recv:%s\n", array.data);
	//find end ctrlf
	printf("carray_uselen(&array)=%d\n", carray_uselen(&array));

	do {
		char *tmptr = NULL, *ctrlf = NULL, *pheadend = NULL;
		int end = carray_uselen(&array);
		tmptr = array.data;
		int isend = 0;
		while (tmptr < array.data + end) {
			if (*tmptr == '\n') {
				if (tmptr - ctrlf <= 2) {
					ctrlf = tmptr;
					isend = 1;
					break;
				}
				ctrlf = tmptr;
			}
			tmptr++;
		}

		if (!isend)
			break;

		//*(ctrlf + 1) = 0;
		pheadend = ctrlf + 1;
		printf("%p %p , %ld \n", ctrlf, array.data,
				pheadend - (char*) array.data);
		carray_usedlen_sub(&array, (void*) pheadend - (void*) array.data);
		printf("carray_remainlen(&array)=%d\n", carray_remainlen(&array));
		//decode rtsp url
		//function: OPTIONS
		//url: rtsp://
	} while (0);

	return EXIT_SUCCESS;
}
