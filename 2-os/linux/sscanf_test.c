/*
 * sscanf_test.c
 *
 *  Created on: 2018年6月16日
 *      Author: cc
 */
#include <stdio.h>
#include <string.h>
//#define NDEBUG
#include <assert.h>

void scanf_test_1() {
	printf("-begin--[%s]---\n", __FUNCTION__);
	char str[100];

	memset(str, 0, sizeof(str));
	sscanf("abcdef", "%2s", str);
	assert(strcmp(str, "ab") == 0);
	printf("%s\n", str);

	memset(str, 0, sizeof(str));
	sscanf("abcdefgABC", "%[a-e]", str);
	assert(strcmp(str, "abcde") == 0);
	printf("%s\n", str);

	printf("-end--[%s]---\n", __FUNCTION__);
}

void scanf_test_timestr() {
	int year = 0;
	int mon = 0;
	int mday = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	int ret = 0;
	printf("-begin--[%s]---\n", __FUNCTION__);
	year = mon = mday = hour = min = sec = 0;
	ret = sscanf("2018/01/02 08:03:02", "%d/%d/%d %d:%d:%d", &year, &mon, &mday,
			&hour, &min, &sec);
	assert(ret == 6);
	printf("ret=%d\n", ret);
	printf("%d/%d/%d %d:%d:%d", year, mon, mday, hour, min, sec);
	printf("\n");

	//"*"表示该输入项读入后不赋予任何变量，即跳过该输入值。
	year = mon = mday = hour = min = sec = 0;
	ret = sscanf("2018-12-02 08:03:02", "%d%*c%d%*c%d %d:%d:%d", &year, &mon,
			&mday, &hour, &min, &sec);
	assert(ret == 6);
	printf("ret=%d\n", ret);
	printf("%d %d %d %d:%d:%d", year, mon, mday, hour, min, sec);
	printf("\n");
	printf("\"%%*\"表示该输入项读入后不赋予任何变量");

	year = mon = mday = hour = min = sec = 0;
	ret = sscanf("2018/01/02 08:03:02", "%d%*c%d%*c%d %d:%d:%d", &year, &mon,
			&mday, &hour, &min, &sec);
	assert(ret == 6);
	printf("ret=%d\n", ret);
	printf("%d/%d/%d %d:%d:%d", year, mon, mday, hour, min, sec);
	printf("\n");

	printf("\"%%[^=]\" 读入任意多的字符,直到遇到\"=\"停止\n");
	year = mon = mday = hour = min = sec = 0;
	ret = sscanf("\n   2018/01/02 08:03:02", "%*[^ ]%d%*c%d%*c%d %d:%d:%d",
			&year, &mon, &mday, &hour, &min, &sec);
	assert(ret == 6);

	printf("ret=%d\n ", ret);
	printf("%d/%d/%d %d:%d:%d", year, mon, mday, hour, min, sec);
	printf("\n");
	printf("-end--[%s]---\n", __FUNCTION__);
}

#include <arpa/inet.h>

void scanf_test_route() {
	char devname[64], flags[16], *sdest, *sgw;
	unsigned long d, g, m;
	int flgs, ref, use, metric, mtu, win, ir;

	const char *ptr_src="enp0s3	00000000	0108A8C0	0003	0	0	100	000000000	0	0 ";

	printf("-begin--[%s]---\n", __FUNCTION__);

	///proc/net/route
	int r = sscanf(ptr_src, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n", devname, &d, &g, &flgs,
			&ref, &use, &metric, &m, &mtu, &win, &ir);
	printf("ret=%d\n",r);

	struct in_addr addr;
	addr.s_addr=g;
	printf("devName:%s,%x->%s\n", devname,d,inet_ntoa(addr));
	//devName:enp0s3,0->192.168.8.1
	printf("-end--[%s]---\n", __FUNCTION__);
}

#ifdef SRC_MAIN
int main_sscanf_test(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	scanf_test_1();
	scanf_test_timestr();
	scanf_test_route();
	assert(0);
	return 0;
}
