
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

//pr_pcpu
long get_cpu_total_occupy()
{
    typedef struct //声明一个occupy的结构体
    {
        unsigned int user; //从系统启动开始累计到当前时刻，处于用户态的运行时间，不包含 nice值为负进程。
        unsigned int nice; //从系统启动开始累计到当前时刻，nice值为负的进程所占用的CPU时间
        unsigned int system; //从系统启动开始累计到当前时刻，处于核心态的运行时间
        unsigned int idle; //从系统启动开始累计到当前时刻，除IO等待时间以外的其它等待时间iowait (12256) 从系统启动开始累计到当前时刻，IO等待时间(since 2.5.41)
    } total_cpu_occupy_t;

    FILE* fd; //定义文件指针fd
    char buff[1024] = { 0 }; //定义局部变量buff数组为char类型大小为1024
    total_cpu_occupy_t t;

    fd = fopen("/proc/stat", "r"); //以R读的方式打开stat文件再赋给指针fd
    fgets(buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
    /*下面是将buff的字符串根据参数format后转换为数据的结果存入相应的结构体参数 */
    char name[16]; //暂时用来存放字符串
    sscanf(buff, "%s %u %u %u %u", name, &t.user, &t.nice, &t.system, &t.idle);

    fprintf(stderr, "====%s:%u %u %u %u====\n", name, t.user, t.nice, t.system, t.idle);

    fclose(fd); //关闭文件fd
    return (t.user + t.nice + t.system + t.idle);
}

void os_get_cpu_use_task(const char* statfile, long* usr, long* sys, long* nic, long* idle)
{
    ///R 18944 18960 18960 0 -1 64 1 0 0 0 0 0 0 0 20 0 11 0
    //1986155 761696256 226 18446744073709551615 4194304
    //4201700 140737488346576 0 0 0 0 16781312 0 0 0 0 -1 2 0 0 0 0 0 6299136 6299912 6303744 140737488347528 140737488347595 140737488347595 140737488351157 0

    struct procps_status_t {
        char state[4]; //R
        long ppid; //父ID
        long pgid; //group
        long sid; //
        long tty;
        long utime;
        long stime;

        long tasknice;
        long start_time;
        long vsz;
        long rss;
    } prev_stat, cstat, *pstat;

    char line_buf[1024];
    char* cp = NULL;
    int n = 0;

    memset(&prev_stat, 0, sizeof(prev_stat));
    memset(&cstat, 0, sizeof(cstat));

    long cputime1 = get_cpu_total_occupy();

    {
        FILE* fp = fopen(statfile, "r");
        if (fp) {
            fgets(line_buf, sizeof(line_buf), fp);
            fclose(fp);
        }
    }
    cp = strrchr(line_buf, ')');
    if (!cp)
        return;
    cp += 2;

    pstat = &prev_stat;
    {
        // utime=34943 该任务在用户态运行的时间，单位为jiffies
        // stime=12605 该任务在核心态运行的时间，单位为jiffies
        // cutime=0 所有已死线程在用户态运行的时间，单位为jiffies
        // cstime=0 所有已死在核心态运行的时间，单位为jiffies
        n = sscanf(cp,
            "%c %u " /* state, ppid */
            "%u %u %d %*s " /* pgid, sid, tty, tpgid */
            "%*s %*s %*s %*s %*s " /* flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
            "%lu %lu " /* utime, stime */
            "%*s %*s %*s " /* cutime, cstime, priority */
            "%ld " /* nice */
            "%*s %*s " /* timeout, it_real_value */
            "%lu " /* start_time */
            "%lu " /* vsize */
            "%lu " /* rss */,
            pstat->state, &pstat->ppid,
            &pstat->pgid, &pstat->sid, &pstat->tty,
            &pstat->utime, &pstat->stime,
            &pstat->tasknice,
            &pstat->start_time,
            &pstat->vsz,
            &pstat->rss);
    }

    sleep(1);
    long cputime2 = get_cpu_total_occupy();
    {
        FILE* fp = fopen(statfile, "r");
        if (fp) {
            fgets(line_buf, sizeof(line_buf), fp);
            fclose(fp);
        }
    }
    cp = strrchr(line_buf, ')');
    if (!cp)
        return;
    cp += 2;
    pstat = &cstat;
    {

        n = sscanf(cp,
            "%c %u " /* state, ppid */
            "%u %u %d %*s " /* pgid, sid, tty, tpgid */
            "%*s %*s %*s %*s %*s " /* flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
            "%lu %lu " /* utime, stime */
            "%*s %*s %*s " /* cutime, cstime, priority */
            "%ld " /* nice */
            "%*s %*s " /* timeout, it_real_value */
            "%lu " /* start_time */
            "%lu " /* vsize */
            "%lu " /* rss */,
            pstat->state, &pstat->ppid,
            &pstat->pgid, &pstat->sid, &pstat->tty,
            &pstat->utime, &pstat->stime,
            &pstat->tasknice,
            &pstat->start_time,
            &pstat->vsz,
            &pstat->rss);
    }

    long ticks1 = prev_stat.utime + prev_stat.stime;
    long ticks2 = cstat.utime + cstat.stime;

    printf("ticks2=%d, ticks1=%d (%d) ", ticks2, ticks1, ticks2 - ticks1);
    printf("cputime2=%d, cputime1=%d (%d)\n", cputime2, cputime1, cputime2 - cputime1);

    {
        //ps计算原理 ps -u, 运行的时间越多占用率越多
        //进程CPU利用率=  进程在CPU上的运行时间 / 进程运行经过的系统时间。
        //进程运行经过的时间= 系统当前时间 - 进程启动时的时间
        //系统当前时间= uptime,   读/proc/uptime
        int seconds_since_boot = 0;
        {
            //第一个参数是代表从系统启动到现在的时间(以秒为单位)：
            //第二个参数是代表系统空闲的时间(以秒为单位)：
            FILE* fp = fopen("/proc/uptime", "r");
            char buff[1024];

            double up = 0;
            if (fp) {
                fgets(buff, sizeof(buff), fp);
                fclose(fp);
                sscanf(buff, "%lf ", &up);
                seconds_since_boot = up;
            }
        }
        unsigned long long total_time; /* jiffies used by this process */
        unsigned long pcpu = 0; /* scaled %cpu, 999 means 99.9% */
        unsigned long long seconds; /* seconds of process life */
        int Hertz = 100;

        total_time = cstat.utime + cstat.stime;

        if(((unsigned long long)seconds_since_boot >= (cstat.start_time / Hertz))) {
            seconds=((unsigned long long)seconds_since_boot - (cstat.start_time / Hertz));
        }       

        if (seconds)
            pcpu = (total_time * 1000ULL / Hertz) / seconds;

        //if (pcpu > 999)
        printf("--%u [%ld] (seconds_since_boot=%d, cstat.start_time=%d) \n", pcpu / 10U, pcpu, seconds_since_boot, cstat.start_time);
        //else
        printf("--%u.%u\n", pcpu / 10U, pcpu % 10U);
    }
    *usr = 100 * (ticks2 - ticks1) / (cputime2 - cputime1);
}

void os_get_cpu_use(long* usr, long* sys, long* nic, long* idle)
{
    typedef struct jiffy_counts_t {
        /* Linux 2.4.x has only first four */
        unsigned long long usr, nic, sys, idle;
        unsigned long long iowait, irq, softirq, steal;
        unsigned long long total;
        unsigned long long busy;
    } jiffy_counts_t;

    const char* statfile = "/proc/stat";

    char line_buf[1024];
    jiffy_counts_t prev_jif, jiffy, *p_jif, *p_prev_jif;
    int ret = 0;

    memset(&prev_jif, 0, sizeof(prev_jif));
    memset(&jiffy, 0, sizeof(jiffy));

    {
        FILE* fp = fopen(statfile, "r");
        if (fp) {
            fgets(line_buf, sizeof(line_buf), fp);
            fclose(fp);
        }
    }
    static const char fmt[] = "cp%*s %llu %llu %llu %llu %llu %llu %llu %llu";

    p_jif = &prev_jif;
    ret = sscanf(line_buf, fmt,
        &p_jif->usr, &p_jif->nic, &p_jif->sys, &p_jif->idle,
        &p_jif->iowait, &p_jif->irq, &p_jif->softirq,
        &p_jif->steal);
    if (ret >= 4) {
        p_jif->total = p_jif->usr + p_jif->nic + p_jif->sys + p_jif->idle
            + p_jif->iowait + p_jif->irq + p_jif->softirq + p_jif->steal;
        /* procps 2.x does not count iowait as busy time */
        p_jif->busy = p_jif->total - p_jif->idle - p_jif->iowait;
    }
    sleep(1);

    {
        FILE* fp = fopen(statfile, "r");
        if (fp) {
            fgets(line_buf, sizeof(line_buf), fp);
            fclose(fp);
        }
    }

    p_jif = &jiffy;
    ret = sscanf(line_buf, fmt,
        &p_jif->usr, &p_jif->nic, &p_jif->sys, &p_jif->idle,
        &p_jif->iowait, &p_jif->irq, &p_jif->softirq,
        &p_jif->steal);

    if (ret >= 4) {
        p_jif->total = p_jif->usr + p_jif->nic + p_jif->sys + p_jif->idle
            + p_jif->iowait + p_jif->irq + p_jif->softirq + p_jif->steal;
        /* procps 2.x does not count iowait as busy time */
        p_jif->busy = p_jif->total - p_jif->idle - p_jif->iowait;
    }

    p_prev_jif = &prev_jif;

    int total_diff = (unsigned)(p_jif->total - p_prev_jif->total);
    if (total_diff == 0)
        total_diff = 1;

#define CALC_STAT(xxx) *xxx = 100 * (double)(p_jif->xxx - p_prev_jif->xxx) / total_diff

    CALC_STAT(usr);
    CALC_STAT(sys);
    CALC_STAT(nic);
    CALC_STAT(idle);
    // CALC_STAT(iowait);
    // CALC_STAT(irq);
    // CALC_STAT(softirq);
}

void file_read(const char* file, char* buf, int len)
{
    int fd = open(file, O_RDONLY);
    int ret = 0;
    if (fd >= 0) {
        ret = read(fd, buf, len - 1);
        close(fd);
    }
    buf[ret] = 0;
}

volatile long pid_run = 0;
void* _RunCpu(void* arg)
{
    int i = 0;
    long count = 0;
    long pid = syscall(SYS_gettid);

    prctl(PR_SET_NAME, "HHHH");
    pid_run = pid;
    while (1) {
        for (i = 0; i < 1000000; i++) {
            count++;
        }
        usleep(100);
    }
}

void* _RunInfo(void* arg)
{
    int index = (int)arg;
    char name[20];
    pthread_detach(pthread_self());
    sprintf(name, "PT-%d", index);
    prctl(PR_SET_NAME, name);

    long usr, sys, nic, idle;

    char path[100];

    sleep(1);
    sprintf(path, "/proc/%d/stat", getpid(), pid_run);
    //
    while (1) {
        os_get_cpu_use_task(path, &usr, &sys, &nic, &idle);
        printf("%d [%s]%%cpu:%4u us, %4u%% sy, %4u ni, %4u id\n", getpid(), name, usr, sys, nic, idle);
        //sleep(1);
    }
    return NULL;
}

int main(int argc, char const* argv[])
{

    int i = 0;
    pthread_t pt;
    printf("pid=%d\n", getpid());

    pthread_create(&pt, NULL, _RunInfo, i);

    pthread_create(&pt, NULL, _RunCpu, 0);

    {
        chdir("/proc");

        char filename[1024];
        char buff[1024 * 8];
        char* ptr = NULL;
        DIR* dir = NULL;
        struct dirent* item = NULL;
        unsigned long pid = 0;

        dir = opendir("/proc");
        if (dir) {
            while (item = readdir(dir)) {
                if (!isdigit(item->d_name[0]))
                    continue;

                pid = strtoul(item->d_name, NULL, 10);

                sprintf(filename, "/proc/%u/", pid);

                struct stat st;
                stat(filename, &st);

                sprintf(filename, "/proc/%u/stat", pid);

                file_read(filename, buff, sizeof(buff));

                ptr = strchr(buff, '(');
                ptr++;
                sscanf(ptr, "%[^)]", filename);

                ptr = strchr(buff, ')');
                *ptr = 0;
                ptr++;

                printf("%u [U:%d,G:%d] [%s]\n", pid, st.st_uid, st.st_gid, filename);
                //printf("   stat:%s\n", ptr);
            }

            closedir(dir);
        }
    }
    {
        long usr, sys, nic, idle;

        os_get_cpu_use(&usr, &sys, &nic, &idle);
        printf("%%cpu:%4u us, %4u%% sy, %4u ni, %4u id\n", usr, sys, nic, idle);
    }
    pause();
    return 0;
}
