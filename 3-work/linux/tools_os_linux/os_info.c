#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/sysinfo.h>

#define __USE_POSIX
#include <time.h>
#include <sys/time.h>

int print_timestamp(int timestamp)
{
    int runday = timestamp / 86400;
    int runhour = (timestamp % 86400) / 3600;
    int runmin = (timestamp % 3600) / 60;
    int runsec = timestamp % 60;
    char str[50];
    char* ptr = str;

    // sprintf(ptr, "%d天%d时%d分%d秒", runday, runhour, runmin, runsec);

    if (runday)
        ptr += sprintf(ptr, "%d天", runday);
    if (runhour)
        ptr += sprintf(ptr, "%d时", runhour);
    if (runmin)
        ptr += sprintf(ptr, "%d秒", runmin);
    ptr += sprintf(ptr, "%d分", runsec);

    printf("%s", str);
    return 0;
}

void os_info_debug_printf()
{
    struct sysinfo info;

    struct timeval tv;
    struct tm t;
    gettimeofday(&tv, NULL);

    sysinfo(&info);
    
    char tmstr[30];
    localtime_r(&tv.tv_sec, &t);
    strftime(tmstr, sizeof(tmstr), "%F %T", &t);

    printf("当前时间:%s ", tmstr);
    print_timestamp(info.uptime);
    printf("\nmem:%ld free:%ld use:%ld %.2f%\n", info.totalram,
        info.freeram, info.totalram - info.freeram,
        (info.totalram - info.freeram) / (info.totalram * 1.0) * 100);

    printf("%d\n", info.procs);
}

#define CONFIG_DEBUG_OS_INFO 1

#if CONFIG_DEBUG_OS_INFO
int main(int argc, const char** argv)
{
    os_info_debug_printf();
    return 0;
}
#endif