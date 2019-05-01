#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    char *cwdstr = getcwd(NULL, 0);
    char *file = "./a.so";

    printf("%s\n", cwdstr);
    if (cwdstr)
        free(cwdstr);

    if (access(file, F_OK) != 0)
        file = "src/functions/a.so";

    void *sohandle = dlopen(file, RTLD_LAZY);
    void *ptr = NULL;
    if (!sohandle)
    {
        fprintf(stderr, "[%s] %s\n", file, dlerror());
        return 0;
    }
    printf("so file open ok\n");

    ptr = dlsym(sohandle, "abc");
    printf("ptr=%p\n", ptr);

    ptr = dlsym(sohandle, "test");
    printf("ptr=%p\n", ptr);
    printf("pid=%d\n", getpid());

    ((void (*)())0)();
    sleep(100);
    return 0;
}
