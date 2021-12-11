/*
 * uuid_build.c
 *
 *  Created on: 2019年11月7日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "uuid_build.h"

int uuid_build(char uuidstr[37]) {
	const char *c = "89ab";
	char *p = uuidstr;
	int n;

	srand((unsigned int)time(NULL));//初始化种子为随机值

	for (n = 0; n < 16; ++n) {
		int b = rand() % 255;
		switch (n) {
		case 6:
			sprintf(p, "4%x", b % 15);
			break;
		case 8:
			sprintf(p, "%c%x", c[rand() % strlen(c)], b % 15);
			break;
		default:
			sprintf(p, "%02x", b);
			break;
		}

		p += 2;
		switch (n) {
		case 3:
		case 5:
		case 7:
		case 9:
			*p++ = '-';
			break;
		}
	}
	*p = 0;
	return 0;
}
