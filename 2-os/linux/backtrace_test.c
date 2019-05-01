#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>

void dump(void)
{
#define BACKTRACE_SIZE 50
    int j, nptrs;
    void *buffer[BACKTRACE_SIZE];
    char **strings;

    nptrs = backtrace(buffer, BACKTRACE_SIZE);

    printf("backtrace() returned %d addresses\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        printf("  [%02d] %s\n", j, strings[j]);

    free(strings);
}

void _sighandle(int signo)
{
    printf(">>>>>>>>>SIGSEGV\n");
    dump();
    signal(signo, SIG_DFL); /* 恢复信号默认处理 */
    raise(signo);           /* 重新发送信号 */
    exit(-1);
}

void exception()
{
    char *ptr=NULL;
    *ptr=5;
    ((void (*)())0)();
}

void test()
{
    printf("test\n");
    exception();
}

int main(int argc, char const *argv[])
{
    signal(SIGSEGV, _sighandle);

    test();
    return 0;
}
