//
//  http_opt.c
//  MusicPlayer
//
//  Created by   CC on 16/5/7.
//  Copyright  2016
//		aut: CC. All rights reserved.
//
#include "http.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "clog.h"

#define curl_easy_setopt_url(curl,urlString) curl_easy_setopt(curl, CURLOPT_URL, urlString)
#define curl_easy_setopt_header(curl,boolFlag) curl_easy_setopt(curl, CURLOPT_HEADER, boolFlag)
#define curl_easy_setopt_writefunction(curl,fun) curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun)
#define curl_easy_setopt_writeData(curl,data) curl_easy_setopt(curl, CURLOPT_WRITEDATA, data)

#define curl_easy_getinfo_response_code(curl, plongValue) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, plongValue)
#define curl_easy_getinfo_content_length_download(curl,pdoubleValue) curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,pdoubleValue)

int http_init() {
	if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
		fprintf(stderr, "curl_global_init() failed\n");
		return -1;
	}
	return 0;
}

void http_cleanup() {
	curl_global_cleanup();
}

enum HTTP_DATA_TYPE {
	IS_CHAR_POINT, IS_FILE_POINT,
};

struct HttpData {
	const char *url;

	union _stream {
		FILE *fp;
		void *data;
	} stream;

	enum HTTP_DATA_TYPE type;
	double dataSize;
	long long content_length;
	long long process;

	void *context;
	void (*hook_backFun)(const char *url, long long content_len, long long process, void *context);
};

static size_t _http_to_header(char *buffer, size_t size, size_t nitems, void *outstream) {
	struct HttpData *data = (struct HttpData*) outstream;
	long long len = 0;
	if (data != NULL) {
		if (sscanf(buffer, "Content-Length: %lld", &len)) {
			data->content_length = len;
			switch (data->type) {
			case IS_CHAR_POINT:
				data->stream.data = malloc(len + 1);
				if (data->stream.data)
					memset(data->stream.data, 0, len + 1);
				else
					data->stream.data = NULL;
				break;
			default:
				break;
			}
		}
	}
	return size * nitems;
}

static size_t _http_to_context(char *buffer, size_t size, //大小
		size_t nitems, //哪一块
		void *outstream) {
	struct HttpData *data = (struct HttpData*) outstream;
//printf("--------------------\r\n");
	if (data != NULL) {
		switch (data->type) {
		case IS_CHAR_POINT:
			if (data->stream.data != NULL)
				memcpy(data->stream.data + data->process, buffer, size * nitems);
			break;
		case IS_FILE_POINT:
			if (data->stream.fp != NULL)
				fwrite(buffer, size, nitems, data->stream.fp);
			break;
		}
		data->process += size * nitems;
	}
	if (data->hook_backFun)
		data->hook_backFun(data->url, data->content_length, data->process, data->context);
	return size * nitems;
}

int http_get_string(const char *url, char **toValue) {
	CURLcode res;
	CURL *curl;
	double downloadFileLenth = 0;
	struct HttpData data;
	memset(&data, 0, sizeof(data));
	data.type = IS_CHAR_POINT;
	data.url = url;

	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}

	curl_easy_setopt_url(curl, url);
//curl_easy_setopt(curl,CURLOPT_TIMEOUT,1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

	curl_easy_setopt_header(curl, 0); //CURLOPT_HEADER 设置为非0将响应头信息同响应体一起传给WRITEFUNCTION
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_to_header);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);

	curl_easy_setopt_writefunction(curl, _http_to_context);
	curl_easy_setopt_writeData(curl, &data);

	long httpcode = 0;
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
		res = curl_easy_getinfo_response_code(curl, &httpcode);

	if (res == CURLE_OK && httpcode == 200) {
		curl_easy_getinfo_content_length_download(curl, &downloadFileLenth);
	}
	curl_easy_cleanup(curl);

	if (toValue != NULL) {
		*toValue = data.stream.data;
	} else {
		if (data.stream.data)
			free(data.stream.data);
	}
	if (res == CURLE_OK && httpcode == 200) {
		return 0;
	}
	return -1;
}

int http_get_file(const char *url, FILE *fp, long long *fileLength,
		void (*backFun)(const char *url, long long content_len, long long process, void *context), void *context) {
	CURLcode res;
	CURL *curl;
	double downloadFileLenth = 0;
	struct HttpData data;
	memset(&data, 0, sizeof(data));

	data.type = IS_FILE_POINT;
	data.stream.fp = fp;
	data.hook_backFun = backFun;
	data.url = url;
	data.context = context;
	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}

	curl_easy_setopt_url(curl, url);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
//curl_easy_setopt(curl, CURLOPT_TIMEOUT,20); //设置cURL允许执行的最长秒数。
//curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); //在发起连接前等待的时间，如果设置为0，则无限等待。

	curl_easy_setopt_header(curl, 0);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_to_header);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);

	curl_easy_setopt_writefunction(curl, _http_to_context);
	curl_easy_setopt_writeData(curl, &data);

	long httpcode = 0;
	res = curl_easy_perform(curl);
	httpcode = res;
	if (res == CURLE_OK)
		res = curl_easy_getinfo_response_code(curl, &httpcode);
	else {
		log_d("get:%s,  ==== err, %d", url, res);
	}

	if (res == CURLE_OK && httpcode == 200) {
		curl_easy_getinfo_content_length_download(curl, &downloadFileLenth);
		if (fileLength != NULL)
			*fileLength = data.content_length;
	}
	curl_easy_cleanup(curl);
	log_d("get:%s,  ==== %d", url, httpcode);

	if (res == CURLE_OK && httpcode == 200) {
		return 0;
	}
	return -1;
}

int http_post_data(const char *url, const char *postData, char **toValue) {
	CURLcode res;
	CURL *curl;
	struct HttpData data;
//struct curl_slist *headers = NULL;

	memset(&data, 0, sizeof(data));
	data.type = IS_CHAR_POINT;

	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url); //url地址
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData); //post参数
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_to_context); //对返回的数据进行操作的函数地址
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data); //这是write_data的第四个参数值
	curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息
	curl_easy_setopt(curl, CURLOPT_HEADER, 1); //将响应头信息和相应体一起传给write_data
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
//curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/Users/zhu/CProjects/curlposttest.cookie");

	long httpcode = -1;
	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
		res = curl_easy_getinfo_response_code(curl, &httpcode);

	if (res == CURLE_OK && httpcode == 200) {

	}
	curl_easy_cleanup(curl);

	if (toValue != NULL) {
		*toValue = data.stream.data;
	} else {
		if (data.stream.data)
			free(data.stream.data);
	}

	if (res == CURLE_OK && httpcode == 200) {
		return 0;
	}
	return -1;
}

int http_post(const char *url, const char *args) {
	CURLcode res;
	CURL *curl;
	struct HttpData data;
	//struct curl_slist *headers = NULL;
	char *path = "/tmp/cache-http-post";
	FILE *fp = fopen(path, "wb");
	struct stat statbuff;

	memset(&data, 0, sizeof(data));
	data.stream.fp = fp;
	data.type = IS_FILE_POINT;

	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_to_context); //对返回的数据进行操作的函数地址
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data); //这是write_data的第四个参数值
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_to_header);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);

	curl_easy_setopt(curl, CURLOPT_URL, url); //url地址
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, args); //post参数

	curl_easy_setopt(curl, CURLOPT_POST, 1); //设置非0表示本次操作为post
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0); //打印调试信息
	curl_easy_setopt(curl, CURLOPT_HEADER, 0); //将响应头信息和相应体一起传给write_data
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

	//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);//
	//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

	long httpcode = 0;
	res = curl_easy_perform(curl);

	if (res == CURLE_OK)
		res = curl_easy_getinfo_response_code(curl, &httpcode);

	curl_easy_cleanup(curl);
	fflush(fp);
	fseek(fp, 0, SEEK_SET);

	int rc = stat(path, &statbuff);
	if (rc == 0) {
		log_d("%d", statbuff.st_size);
	}
	fclose(fp);

	log_d("res=%d, http code=%d", res, httpcode);
	if (res == CURLE_OK && httpcode == 200) {
		return 0;
	}
	remove(path);
	return -1;
}

static size_t _http_write_context(char *buffer, size_t size, //大小
		size_t nitems, //哪一块
		void *outstream) {
	fwrite(buffer, size, nitems, stdout);
	fflush(stdout);
	return size * nitems;
}

int http_post_file(const char *url, const char *event, const char *uid, const char *filename, const char *filepath, int seq,
		const char* fileType) {
	CURL *curl;
	CURLcode res;
	char seq_str[10];
	sprintf(seq_str, "%d", seq);

	curl = curl_easy_init();
	long httpcode = 0;
	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() failed\n");
		return -1;
	}

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	//struct curl_slist *headerlist=NULL;
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "uid", CURLFORM_COPYCONTENTS, uid, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "event", CURLFORM_COPYCONTENTS, event, CURLFORM_END);
	/* Fill in the file upload field. This makes libcurl load data from
	 the given file name when curl_easy_perform() is called. */
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, filename, CURLFORM_END);

	if (filepath != NULL && strlen(filepath) > 0)
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "file", CURLFORM_FILE, filepath, CURLFORM_END);

	if (seq > 0)
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "seq", CURLFORM_COPYCONTENTS, seq_str, CURLFORM_END);

	if (fileType)
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "fileType", CURLFORM_COPYCONTENTS, fileType, CURLFORM_END);

	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_write_context); //对返回的数据进行操作的函数地址
	//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data); //这是write_data的第四个参数值
	//  curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size); // upload file size--- Content-Length: size
	//   curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, 20*1000);    // speed limit
#if 0
	if(strncmp(url,"https",5)==0) { //need ssl
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);//设定为不验证证书和HOST
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl,CURLOPT_SSLCERT,"/opt/app/1-cert.pem");
		// curl_easy_setopt(curl,CURLOPT_SSLCERTPASSWD,"password");
		curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
		curl_easy_setopt(curl,CURLOPT_SSLKEY,"/opt/app/1-key.pem");
		// curl_easy_setopt(curl,CURLOPT_SSLKEYPASSWD,"password");
		curl_easy_setopt(curl,CURLOPT_SSLKEYTYPE,"PEM");
	}
#endif

	res = curl_easy_perform(curl);
	if (res == CURLE_OK)
		res = curl_easy_getinfo_response_code(curl, &httpcode);
	log_d("res=%d, http code=%d", res, httpcode);
	if (res != 0) {
		printf("errcode %d,  %s\n", res, curl_easy_strerror(res));
	} else {
		printf("http push msg success\n");
	}

	curl_formfree(formpost);
	curl_easy_cleanup(curl);

	if (res == CURLE_OK && httpcode == 200) {
		return 0;
	}
	return -1;
}
