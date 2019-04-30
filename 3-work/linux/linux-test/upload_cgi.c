/*
 * main.c
 *
 *  Created on: 2018年6月19日
 *      Author: cj
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

//extern char ** environ;
//char ** envir = environ;
//while (*envir) {
//	printf("%s\n", *envir);
//	envir++;
//}
//---------------------------------------------------------------------------------------------------------------------------------------------------
//PATH=/sbin:/usr/sbin:/bin:/usr/bin
//HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
//HTTP_ACCEPT_CHARSET=
//HTTP_ACCEPT_ENCODING=gzip, deflate
//HTTP_ACCEPT_LANGUAGE=en-US,en;q=0.5
//HTTP_AUTHORIZATION=
//HTTP_CONNECTION=keep-alive
//HTTP_COOKIE=
//HTTP_HOST=192.168.1.100
//HTTP_REFERER=http://192.168.1.100/firmware.html
//HTTP_USER_AGENT=Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:53.0) Gecko/20100101 Firefox/53.0
//CONTENT_TYPE=multipart/form-data; boundary=---------------------------1268134501272185836714431268
//CONTENT_LENGTH=246
//GATEWAY_INTERFACE=CGI/1.1
//SERVER_SOFTWARE=uhttpd
//SCRIPT_NAME=/cgi-bin/upload
//SCRIPT_FILENAME=/www/cgi-bin/upload
//DOCUMENT_ROOT=/www
//QUERY_STRING=
//REQUEST_URI=/cgi-bin/upload
//SERVER_PROTOCOL=HTTP/1.1
//REQUEST_METHOD=POST
//REDIRECT_STATUS=200
//SERVER_NAME=192.168.1.100
//SERVER_ADDR=192.168.1.100
//SERVER_PORT=80
//REMOTE_HOST=192.168.1.205
//REMOTE_ADDR=192.168.1.205
//REMOTE_PORT=36819

#define BUF_LEN  2048

void _error(int result, const char *msg) {
	printf("Content-Type: application/json\n\n");
	printf("{\"result\":%d, \"msg\":\"%s\"}", result, msg);
}

int _getline(char *buff, int buflen) {
	unsigned char tmp = 0;
	char *pos = buff;

	while (buflen > 0) {
		fread(&tmp, 1, 1, stdin);
		if (tmp == '\n') {
			*pos = tmp;
			pos++;
			break;
		}
		*pos = tmp;
		pos++;
		buflen--;
	}
	return pos - buff;
}

//文件上传数据包格式(内容后面有两个换行),头和尾部为分界线
//	-----------------------------51670742787654471165200430
//	Content-Disposition: form-data; name="fileUpload"; filename="md5sums"
//	Content-Type: application/octet-stream
//
//	3423432423432423423411111
//
//	-----------------------------51670742787654471165200430--
//
void read_content(int content_length, const char *boundary) {
	char buff[BUF_LEN];
	char buff_next[BUF_LEN];
	int i = 0;
	int rlen = 0, rlen_next = 0;
	int rlensum = 0;
	char msg[100];

	FILE *fp = NULL;

	while (i < 5) {
		i++;
		memset(buff, 0, sizeof(buff));
		rlensum += _getline(buff, sizeof(buff));
		if (buff[0] == '\n' || (buff[0] == '\r' && buff[1] == '\n')) {
			break;
		}
	}

	remove("/tmp/http_file.tmp");
	fp = fopen("/tmp/http_file.tmp", "wb");

	while (rlensum < content_length) {
		memset(buff, 0, sizeof(buff));
		rlen = _getline(buff, sizeof(buff));
		rlensum += rlen;

		if ((buff[rlen - 1] == '\n') || (buff[rlen - 2] == '\r' && buff[rlen - 1] == '\n')) {

			rlen_next = _getline(buff_next, sizeof(buff_next));
			rlensum += rlen_next;

			if (strstr(buff_next, boundary) != NULL) { //分界 //这样会多一个换行[\r\n]怎么办
				sprintf(msg, "buff next end");

				if (buff[rlen - 2] == '\r' && buff[rlen - 1] == '\n')
					fwrite(buff, 1, rlen - 2, fp);
				else if (buff[rlen - 1] == '\n')
					fwrite(buff, 1, rlen - 1, fp);
				break;
			}
			fwrite(buff, 1, rlen, fp);
			fwrite(buff_next, 1, rlen_next, fp);
		} else {

			if (strstr(buff, boundary) != NULL) { //分界 //这样会多一个换行[\r\n]怎么办
				sprintf(msg, "buff  end");
				break;
			}
			fwrite(buff, 1, rlen, fp);
		}
	}
	fclose(fp);

	printf("Content-Type: application/json\n\n");
	printf("{\"result\":%d, \"msg\":\"%s\"}", 0, msg);

//	while (rlensum < content_length) {
//		rlen = fread(buff, 1, sizeof(buff), stdin);
//		rlensum += rlen;
//	}
}

int main(int argc, char **argv) {
	int content_length;/*标准输入内容长度*/
	const char *content_type = NULL;
	const char *boundary = NULL; //分界线

	content_type = getenv("CONTENT_TYPE");
	content_length = atoi((char*) getenv("CONTENT_LENGTH"));

	if (strncasecmp(content_type, "multipart/form-data", strlen("multipart/form-data")) != 0) {
		_error(-1, "content type err! not multipart/form-data");
		exit(0);
	}

	if (content_length == 0) {
		_error(-1, "content length is 0");
		exit(0);
	}

	boundary = strstr(content_type, "boundary=");
	if (boundary) {
		boundary += strlen("boundary=");
	}

	read_content(content_length, boundary);
	return 0;
}
