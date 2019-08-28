#ifndef _BUILD_TIME_H
#define _BUILD_TIME_H

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <time.h>

static inline void build_time(char str[50])
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	sprintf(str, "%s %s", __DATE__, __TIME__);

	strptime(str, "%b%d%Y %H:%M:%S", &tm); //str -> tm: 
	strftime(str, sizeof(str), "%F %T", &tm); //tm -> str: %Y-%m-%d %H:%M:%S
}

#endif
