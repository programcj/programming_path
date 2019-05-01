/*
 * util.c
 *
 *  Created on: 2015年12月1日
 *      Author: cj
 *      Email:593184971@qq.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "clog.h"
#include "stringex.h"
#include "tools.h"

#if CONFIG_HAVE_UUID
#include <uuid/uuid.h>
#endif

//取文件大小
unsigned long tools_file_get_size(const char *path) {
//	FILE *f = fopen(fileName, "rb");
	unsigned long filesize = 0;
//
//	if (f != NULL) {
//		fseek(f, 0, SEEK_END);
//		len = ftell(f);
//		fseek(f, 0, SEEK_SET);
//		fclose(f);
//		f = NULL;
//	}
	struct stat statbuff;
	int rc = stat(path, &statbuff);
	if (rc < 0) {
		return filesize;
	} else {
		filesize = statbuff.st_size;
	}
	return filesize;
}

//从文件中读取size大小的数据到data中
int tools_file_read(const char *fileName, int index, unsigned char *data, int size) {
	FILE *f = fopen(fileName, "rb");
	int len = 0;
	if (f != NULL) {
		fseek(f, index, SEEK_SET);
		len = fread(data, 1, size, f);
		fclose(f);
	}
	return len;
}

unsigned short tools_crc16(unsigned char *pDataAddr, int bDataLen) {
	unsigned char WorkData;
	unsigned char bitMask;
	unsigned char NewBit;
#define CRC_POLY        0x1021
#define CRC_INIT_VALUE    0x1D0F
	unsigned short crc = 0x1D0F;
	//printf("ZW_CheckCrc16: bDataLen = %u\r\n", bDataLen);
	while (bDataLen--) {
		WorkData = *pDataAddr++;
		for (bitMask = 0x80; bitMask != 0; bitMask >>= 1) {
			/* Align test bit with next bit of the message byte, starting with msb. */
			NewBit = ((WorkData & bitMask) != 0) ^ ((crc & 0x8000) != 0);
			crc <<= 1;
			if (NewBit) {
				crc ^= CRC_POLY;
			}
		} /* for (bitMask = 0x80; bitMask != 0; bitMask >>= 1) */
	}
	return crc;
}

unsigned short tools_file_crc16(const char *fileName) {
	FILE *f = fopen(fileName, "rb");
	int len = 0;
	unsigned char *data = NULL;
	unsigned short crc16 = 0;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (len > 0) {
			data = (unsigned char*) malloc(len + 1);
			memset(data, 0, len + 1);
			fread(data, 1, len, f);
			crc16 = tools_crc16(data, len);
			free(data);
		}
		fclose(f);
		f = NULL;
	}
	return crc16;
}

int tools_md5sum_calc(const char *filePath, char md5str[33]) {
	FILE *fp = NULL;
	char cmdLine[200];
	char buffer[33];
	memset(cmdLine, 0, sizeof(cmdLine));
	strcpy(cmdLine, "md5sum ");
	strcat(cmdLine, filePath);

	fp = popen(cmdLine, "r");
	memset(buffer, 0, sizeof(buffer));
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		break;
	}
	pclose(fp);
	if (strlen(buffer) == 0) {
		return -1;
	}
	strcpy(md5str, buffer);
	return 0;
}

int tools_readcmdline(const char *cmdline, char *buff, int maxsize) {
	FILE *fp = NULL;
	int readlen = 0;
	int linelen = 0;
	char *next = NULL;

	fp = popen(cmdline, "r");

	while (fgets(buff, maxsize - readlen, fp) != NULL && readlen < maxsize) {
		linelen = strlen(buff);
		if (linelen == 0) {
			break;
		}
		next = strchr(buff, '\n');
		if (next)
			*next = 0;
		readlen += linelen;
		buff += linelen;
	}
	pclose(fp);
	return 0;
}

int tools_read_file(const char *fileName, char **buf) {
	int result = -1;
	FILE *fp;
	fp = fopen(fileName, "r");
	if (NULL != fp) {
		int len;

		fseek(fp, 0, SEEK_END); //将文件内部指针放到文件最后面
		len = ftell(fp); //读取文件指针的位置，得到文件字符的个数
		if (len > 0) {
			*buf = (char *) malloc(len);
			if (*buf != NULL) {
				fseek(fp, 0, SEEK_SET);
				fread(*buf, len, 1, fp);
				result = 0;
			}
		}
		fclose(fp);
	}
	fp = NULL;
	return result;
}

int tools_read_file_tobuff(const char *filename, char *buff, int maxsize) {
	int result = -1;
	FILE *fp;
	fp = fopen(filename, "r");
	if (NULL != fp) {
		if (buff != NULL) {
			fseek(fp, 0, SEEK_SET);
			fread(buff, maxsize, 1, fp);
			result = 0;
		}
		fclose(fp);
	}
	fp = NULL;
	return result;
}

int tools_write_file(const char *fileName, const char *buf) {
	int result = -1;
	FILE *fp;
	fp = fopen(fileName, "w+");
	if (NULL != fp) {
		fwrite(buf, strlen(buf), 1, fp);
		fflush(fp);
		result = 0;
	}
	fclose(fp);
	fp = NULL;
	return result;
}

//文件是否存在,存在返回>0;  不存在返回0
int tools_file_exists(const char *fileName) {
	return 0 == access(fileName, F_OK) ? 1 : 0;
}

/**/
int tools_file_isdir(const char *file_path)
{
	struct stat stbuf;
	if(access(file_path, F_OK)!=0)
		return -1;

	stat(file_path, &stbuf);

	return S_ISDIR(stbuf.st_mode);
}

/**/
int tools_file_isreg(const char *file_path)
{
	struct stat stbuf;
	if(access(file_path, F_OK)!=0)
		return -1;

	stat(file_path, &stbuf);
	// S_ISLNK(st_mode):是否是一个连接.
	// S_ISREG是否是一个常规文件.
	// S_ISDIR是否是一个目录
	// S_ISCHR是否是一个字符设备.
	// S_ISBLK是否是一个块设备
	// S_ISFIFO是否是一个FIFO文件.
	// S_ISSOCK是否是一个SOCKET文件. 
	return S_ISREG(stbuf.st_mode);
}

void tools_uuid(char str[33]) {
	#if CONFIG_HAVE_UUID

	int i = 0, j = 0;
	uuid_t uuid;
	char uuidStr[37];
	memset(uuidStr, 0, sizeof(uuidStr));

	uuid_generate(uuid);
	uuid_unparse(uuid, uuidStr);
	while (i < 36) {
		if (uuidStr[i] != '-')
			uuidStr[j++] = uuidStr[i++];
		else
			i++;
	}
	uuidStr[j] = '\0';
	strncpy(str, uuidStr, 33);
	str[32] = '\0';

	#endif
}

//1 000 000 (百万)微秒 = 1秒
//毫秒当前时间 类拟java的System.currentTimeMillis();
uint64_t tools_current_time_millis() {
	struct timeval tv;
	uint64_t t;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	t *= 1000;
	t += tv.tv_usec / 1000;
	return t;
}

uint64_t tools_os_runtime(int secflag)
{
	struct timespec times =
	{ 0, 0 };
	uint64_t time;
	clock_gettime(CLOCK_MONOTONIC, &times);

	if (1 == secflag)
		time = times.tv_sec;
	else //ms
		time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	return time;
}

void tools_timeget_local(char *timestring) {
	time_t timep;
//	struct tm *p;
	struct tm t;
	time(&timep);
	localtime_r(&timep, &t);
//	p = localtime(&timep); /*取得当地时间*/
	//sprintf(timestring, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	sprintf(timestring, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
}

void tools_timeget_local_tim(char tim[9]) {
	time_t timep;
	struct tm t;
	time(&timep);
	localtime_r(&timep, &t);
	sprintf(tim, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
}

void tools_timeget_otc(char timestring[30]) {
	time_t timep;
	//struct tm *p;
	struct tm t;
	char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	time(&timep);
	//p = gmtime(&timep);
	gmtime_r(&timep, &t);
	//sprintf(timestring, "%04d-%02d-%02d %s %02d:%02d:%02d", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, wday[p->tm_wday], p->tm_hour,
	//		p->tm_min, p->tm_sec);
	sprintf(timestring, "%04d-%02d-%02d %s %02d:%02d:%02d", (1900 + t.tm_year), (1 + t.tm_mon), t.tm_mday, wday[t.tm_wday], t.tm_hour,
			t.tm_min, t.tm_sec);
}

//时间来自于字符串
int tools_timevalcur_fromstr(struct timeval *ptv, const char *tmstr)
{
	int year = 0;
	int mon = 0;
	int mday = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	const char *next = NULL;
	struct tm _tm;
	time_t timep;
	time_t timer;
	int ret = 0;

	time(&timer);
	localtime_r(&timer, &_tm);

	//2019/03/28 14:20:23
	if (strlen(tmstr) < 5)
		return -1;

	ret = sscanf(tmstr, "%d%*c%d%*c%d %d%*c%d%*c%d", &year, &mon, &mday,
			&hour, &min, &sec);

	if (ret == 3)
	{
		if (year > 30)
		{
			_tm.tm_year = year - 1900;
			_tm.tm_mon = mon - 1;
			_tm.tm_mday = mday;
		}
		else
		{
			_tm.tm_hour = year;
			_tm.tm_min = mon;
			_tm.tm_sec = mday;
		}
	}
	else if (ret == 6)
	{
		_tm.tm_year = year - 1900;
		_tm.tm_mon = mon - 1;
		_tm.tm_mday = mday;

		_tm.tm_hour = hour;
		_tm.tm_min = min;
		_tm.tm_sec = sec;
	}

	timep = mktime(&_tm);
	ptv->tv_sec = timep;
	ptv->tv_usec = 0;
	return 0;
}

int tools_config_file_getstr(const char *file_path, const char *name, char *valuestr, int vlen)
{
	char buff[1024];
	char *vpoint;
	char *value = valuestr;

	FILE *file = fopen(file_path, "r");
	if(!file)
		return -1;
	do {
		memset(buff, 0, sizeof(buff));
		fgets(buff, 1024, file);
		if (strncasecmp(name, buff, strlen(name)) == 0) {
			vpoint = strchr(buff, '=');
			vpoint++;
			while (*vpoint == ' ')
				vpoint++;

			while (*vpoint != 0 && *vpoint != '\r' && *vpoint != '\n'
					&& *vpoint != '#' && *vpoint != ' ' && vlen > 0) {
				*value++ = *vpoint++;
				vlen--;
			}
			*value = 0;
			fclose(file);
			return 0;
		}
	} while (!feof(file));
	fclose(file);

	return -1;
}

int tools_ipv4strcheck(const char *ipstring, uint32_t *ipv4)
{
	/**
	 * 最短 0.0.0.0 7位
	 * 最长 000.000.000.000 15位
	 */
	//struct in_addr ipv4;
	int i = 0;
	char buff[15];
	const char *ptr = NULL, *next = NULL;
	char *pv = (char*) ipv4;

	int len = strlen(ipstring);
	if (len < 7 || len > 15)
	{
		fprintf(stderr, "length err:%s\n", ipstring);
		return -1;
	}

	ptr = ipstring;

	i = 0;
	for (ptr = ipstring; *ptr; ptr++)
	{
		if (*ptr == '.')
		{
			i++;
			continue;
		}
		if (*ptr < '0' || *ptr > '9') //48-57
		{
			fprintf(stderr, "char err:%c\n", *ptr);
			return -1;
		}
	}

	if (i != 3)
	{
		fprintf(stderr, "need '.' [xx.xxx.xxx.xxx]\n");
		return -1;
	}
	// <=255
	ptr = ipstring;

	for (i = 0; i < 4; i++)
	{
		next = strchr(ptr, '.');
		memset(buff, 0, sizeof buff);
		//log_d("next=%s", next);
		if (next == NULL && i == 3)
			next = ipstring + len;
		if (next == NULL)
		{
			fprintf(stderr, "数据问题\n");
			return 0;
		}

		strncpy(buff, ptr, next - ptr);
		if (atoi(buff) > 255 || strlen(buff) == 0)
		{
			fprintf(stderr, "数据大小问题:buf=%s\n", buff);
			return -1;
		}
		ptr = next + 1;
		if (pv)
			pv[i] = atoi(buff);
	}
	return 0;
}


static uint64_t tools_gettimeofday2int(int usecnumber)
{
	struct timeval tv;
	uint64_t v = 0;
	gettimeofday(&tv, NULL);
	v = tv.tv_sec;

	if (usecnumber == 3)
	{
		v *= 1000;
		v += tv.tv_usec / 1000;
	}
	if (usecnumber == 4)
	{
		v *= 10000;
		v += tv.tv_usec / 100;
	}
	if (usecnumber == 5)
	{
		v *= 100000;
		v += tv.tv_usec / 10;
	}
	return v;
}