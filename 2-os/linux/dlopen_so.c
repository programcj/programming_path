#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//gcc -fvisibility=hidden -fPIC -shared dlopen_so.c -o a.so
//readelf -s a.so > tmp.txt
//objdump -T a.so
//dpkg-query -S $(which strings)

//避免僵尸进程
//1, wait()或waitpid()
//2, 让僵尸进程变成孤儿进程, fork fork
//3, 父进程 signal(SIGCHLD, SIG_IGN); //忽略SIGCHLD信号
//4, 捕获 SIGCHLD 信号, waitpid(-1,


__attribute__((visibility("hidden"))) void abc()
{
    printf("%s\n", __FUNCTION__);
}

__attribute__((visibility("default"))) void test()
{
    printf("%s\n", __FUNCTION__);
}

void test1()
{
    printf("%s\n", __FUNCTION__);
}