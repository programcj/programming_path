#define __GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>

static unsigned long kscale(unsigned long b, unsigned long bs)
{
    return (b * (unsigned long long)bs + 1024 / 2) / 1024;
}

int main(int argc, char const *argv[])
{
    struct statfs s;
    long sdcard_free_kb;

    statfs("/", &s);
    if (s.f_blocks > 0)
    {
        long blocks_used = s.f_blocks - s.f_bfree;
        long blocks_percent_used = 0;
        if (blocks_used + s.f_bavail)
        {
            blocks_percent_used = (blocks_used * 100ULL + (blocks_used + s.f_bavail) / 2) / (blocks_used + s.f_bavail);
        }
        printf(" %9lu %9lu %9lu %3u%%\n",
               kscale(s.f_blocks, s.f_bsize),             // 总数
               kscale(s.f_blocks - s.f_bfree, s.f_bsize), //已经使用
               kscale(s.f_bavail, s.f_bsize),             //可用
               blocks_percent_used);

        sdcard_free_kb = kscale(s.f_bavail, s.f_bsize);

        printf("free:%dkb\n", sdcard_free_kb);
    }
    return 0;
}
