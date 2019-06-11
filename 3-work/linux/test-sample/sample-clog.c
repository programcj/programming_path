/*
 * sample-clog.c
 *
 *  Created on: 2019年6月11日
 *      Author: cc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../public_tools/clog.h"

int main(int argc, char **argv) {
	log_setlevel(CLOG_ALL);
	log_i("start\n");
	log_d("start..\n");
	log_setlevel(CLOG_INFO);
	log_i("start\n");
	log_d("start..\n");
	return 0;
}

