#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    char timestr[100];
    struct tm t;
    time_t curtime;

    curtime=time(NULL);
    localtime_r(&curtime, &t);

    strftime(timestr, sizeof(timestr), "[%F %T]", &t);
    printf("timestr:%s\n", timestr);
    return 0;
}
