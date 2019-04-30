/*
 * test.c
 *
 *  Created on: 2019年4月12日
 *      Author: cj
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>

int main(int argc, char **argv) {
	DIR *dir = opendir("/usr/include");
	if (dir) {
		const char *ptr=NULL;
		struct dirent *item = NULL;
		while ((item = readdir(dir)) != NULL) {
			ptr=strrchr(item->d_name,'.');
			if(ptr){
				if(0==strcmp(ptr,".h")){
					printf(" <%s>\n", item->d_name);
				}
			}
		}
		closedir(dir);
	}
	return EXIT_SUCCESS;
}
