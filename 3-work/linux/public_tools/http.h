/*
 * http.h
 *
 *  Created on: 2016年2月19日
 *      Author: cj
 */

#ifndef COMMON_HTTP_H_
#define COMMON_HTTP_H_

#include <stdio.h>

int http_init();

void http_cleanup();

//从http的URL 地址, 读取字符串
int http_get_string(const char *url, char **toValue);

int http_get_file(const char *url, FILE *fp, long long *fileLength,
		void (*backFun)(const char *url, long long content_len, long long process, void *context), void *context);

int http_post_data(const char *url, const char *postData, char **toValue);

int http_post(const char *url, const char *args);

/**
 * 向服务器推送文件
 * @url: 地址
 * @event: 事件
 * @uid: uid - TutkID
 * @filename:文件名称
 * @resultflag: null
 * @filepath:视频文件目录
 * @seq: 移动值
 * @fileType:文件类型(0:h264, 1:mp4)
 */
int http_post_file(const char *url, const char *event, const char *uid, const char *filename, const char *filepath, int seq,
		const char *fileType);

//从http的URL 地址, 读取字符串，
//int http_get(const char *url, char *buffer, int buffLen, int *readLen);

//从http的URL下载文件
//int http_down_file(const char *url, const char *fileName);

#endif /* COMMON_HTTP_H_ */
