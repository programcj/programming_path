#define _GNU_SOURCE

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/sysinfo.h>

#define __USE_POSIX
#include <sys/time.h>
#include <time.h>

int print_timestamp(int timestamp)
{
    int runday = timestamp / 86400;
    int runhour = (timestamp % 86400) / 3600;
    int runmin = (timestamp % 3600) / 60;
    int runsec = timestamp % 60;
    char str[50];
    char *ptr = str;

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

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#define __USE_MISC 1
#include <net/if.h>

int net_info()
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *ifList;

    if (getifaddrs(&ifList) < 0)
    {
        return -1;
    }

    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            printf("\n>>> interfaceName: %s\n", ifa->ifa_name);

            sin = (struct sockaddr_in *)ifa->ifa_addr;
            printf(" IP: %s ", inet_ntoa(sin->sin_addr));

            sin = (struct sockaddr_in *)ifa->ifa_dstaddr;
            printf("广播: %s ", inet_ntoa(sin->sin_addr));

            sin = (struct sockaddr_in *)ifa->ifa_netmask;
            printf("掩码: %s", inet_ntoa(sin->sin_addr));
            printf("\n mac:");

            // show_mac(ifa->ifa_name,
            //     inet_ntoa(
            //         ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr));
        }
        if (ifa->ifa_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *sin = (struct sockaddr_in6 *)ifa->ifa_addr;
            char str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &sin->sin6_addr, str, sizeof(str));
            printf("IPV6:%s \n", str);
        }
    }
    freeifaddrs(ifList);
    return 0;
}

#include <sys/utsname.h>

void os_uname()
{
    struct utsname n;
    uname(&n);
    printf("%s %s,n:%s,r:%s version:%s\n", n.sysname, n.machine, n.nodename, n.release, n.version);
}

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int os_uname2id(const char *uname)
{
    return -1;
}

int os_uid2uname(int uid, char *uname, int size)
{
    FILE *stream = NULL;
    char buff[256];
    stream = fopen("/etc/group", "r");
    if (stream)
    {
        char *tmptr = NULL;
        char *item = NULL;
        char *unameptr = NULL;
        while (fgets(buff, sizeof(buff), stream))
        {
            if (buff[0] == '#')
                continue;
            item = __strtok_r(buff, ":", &tmptr);
            if (item)
            {
                unameptr = item;
                item = __strtok_r(NULL, ":", &tmptr);
                if (item)
                {
                    item = __strtok_r(NULL, ":", &tmptr);
                    if (item)
                    {
                        if (atoi(item) == uid)
                        {
                            strncpy(uname, unameptr, size);
                        }
                    }
                }
            }
        }

        fclose(stream);
    }
}

struct netsockinfo
{
    char procpath[256];
    int pid;
    int uid; //userid
    int ppid;
    int sock_inode;
};

static void proc_thread_fd(const char *path)
{
    DIR *dir = NULL;
    struct dirent *diritem = NULL;
    char pathtmp[256];
    char pathlink[256];
    char uname[20];

    dir = opendir(path);
    if (dir)
    {
        struct stat st;
        stat(path, &st);
        os_uid2uname(st.st_uid, uname, sizeof(uname));

        while (diritem = readdir(dir))
        {
            if (strcmp(diritem->d_name, ".") == 0 || strcmp(diritem->d_name, "..") == 0)
                continue;
            if (DT_LNK != diritem->d_type)
                continue;

            //
            snprintf(pathtmp, sizeof(pathtmp), "%s/%s", path, diritem->d_name);
            memset(pathlink, 0, sizeof(pathlink));
            readlink(pathtmp, pathlink, sizeof(pathlink));
            if (strncmp(pathlink, "socket:", 6))
                continue;
            printf("fd-inode:uid:%d(%s) %s -> %s\n", st.st_uid, uname,
                   pathtmp, pathlink);
        }
        closedir(dir);
    }
}

void proc_thread()
{
    DIR *dir = NULL;
    struct dirent *diritem = NULL;
    char path[256];
    char uname[20];
    char path_tmp[256];
    strcpy(path, "/proc");

    char tname[1024];


    dir = opendir(path);
    if (dir)
    {
        while (diritem = readdir(dir))
        {
            if (strcmp(diritem->d_name, ".") == 0 || strcmp(diritem->d_name, "..") == 0)
                continue;
            if (diritem->d_type != DT_DIR)
                continue;
            if (!isdigit(diritem->d_name[0]))
                continue;

            //printf("%s ", diritem->d_name);
            pid_t pid = atoi(diritem->d_name);

            snprintf(path_tmp, sizeof(path_tmp), "%s/%d/cmdline", path, pid);
            {
                FILE *fp = fopen(path_tmp, "r");
                if (fp)
                {

                    fseek(fp, 0, SEEK_END);
                    fgets(tname, sizeof(tname), fp);
                    printf("task:%s\n", tname);

                    fclose(fp);
                }
            }

            snprintf(path_tmp, sizeof(path_tmp), "%s/%d/fd", path, pid);
            proc_thread_fd(path_tmp);

            //snprintf(path_tmp, sizeof(path_tmp), "%s/%d/task", path, pid);
            //proc_thread_task(path_tmp);
            // struct stat st;
            // stat(diritem->d_name, &st);
            // os_uid2uname(st.st_uid, uname, sizeof(uname));
            // printf("u:%s \n", uname);
            //printf("id=%d ", pid);
        }

        closedir(dir);
    }
}

#if CONFIG_DEBUG_OS_INFO
int main(int argc, const char **argv)
{
    os_info_debug_printf();

    net_info();
    os_uname();

    proc_thread();

    //printf("uname:%s\n", uname);
    return 0;
}
#endif