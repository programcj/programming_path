/*
 * printf_test.c
 *
 *  Created on: 2019年4月14日
 *      Author: cj
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	char *str = "abcdefghijklmn";

//	%[m][.n]s
//	m   输出最小宽度，单位字节，若str长度不足m，前补空格
//	.n  仅输出字符串str的前n位
	int ret = 0;
	ret = printf("%%g=%g\n", 1.08);
	printf("    ret=%d\n", ret);

	ret = printf("%%G=%G\n", 1.0809);
	printf("    ret=%d\n", ret);

	ret = printf("%%e=%e\n", 1.0809);
	printf("    ret=%d\n", ret);

	ret = printf("ascii 0=%d\n", '0');
	printf("    ret=%d\n", ret);
	ret = printf("%%3s=%3s\n", str);
	printf("    ret=%d\n", ret);

	ret = printf("%%3.0s=%3.0s\n", str); //3空格
	printf("    ret=%d\n", ret);

	ret = printf("%.2s\n", str); //
	printf("    ret=%d\n", ret);

	ret = printf("%0.5s\n", str);
	printf("    ret=%d\n", ret);

	printf("%02X\n", 0x1A);
	printf("%02x\n", 0x1a);

	printf("hello 999");
	printf("\r");
	printf("1===1\n");
	printf("\n");

	return EXIT_SUCCESS;
}
