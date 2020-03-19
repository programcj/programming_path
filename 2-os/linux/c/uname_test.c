#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

int main(int argc, char const *argv[])
{
    struct utsname osname;
    uname(&osname);

    printf("sysname=%s, machine=%s, nodename=%s\n", osname.sysname, osname.machine, osname.nodename);
    return 0;
}
