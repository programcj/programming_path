#define __USE_POSIX

#include "../public_tools/tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//spptr min size >=2, len min >=1
int tools_strspilts(char *buff, char *spchr, char **spptr, int len)
{
    char *tmp = NULL;
    char *ptr = NULL;
    int splen=len;
    int spcount=0;
    ptr = strtok_r(buff, spchr, &tmp);
    if(!ptr)
        return -1;
    while (splen && ptr)
    {
        *spptr = ptr;
        spptr++;
        *spptr = tmp;
        spcount++;
        splen--;

        if (splen)
            ptr = strtok_r(NULL, spchr, &tmp);
    }
    //*len=spcount;
    return spcount;
}

int main(int argc, char const *argv[])
{
    char name[100];
    system("echo name=admin >/tmp/config.txt");
    tools_config_file_getstr("/tmp/config.txt", "name", name, sizeof name);
    printf("name=[%s]\n", name);

    {
        char buff[100] = "name:admin";

        char *ptr = strtok(buff, ":");
        while (ptr)
        {
            printf("%s\n", ptr);
            ptr = strtok(NULL, ":");
        }

        printf("buff=%s\n", buff);
    }
    printf("=========================\n");
    {
        char buff1[] = "name:admin:34324:312";

        char *ptr[5] = {0};
        int len = 1;
        int i = 0;

        len=tools_strspilts(buff1, ":", ptr, 1);
        printf("len=%d\n", len);

        for (i = 0; i < len; i++)
            printf("[%s]\n", ptr[i]);

        printf("[%s]\n", ptr[i]);
    }

    {
        char *s[3]={"aaa", "bbbb", "cccccc"};
        char *p=s;

        printf("=================\n");
        for(int i=0;i<3;i++){
            printf("%p\n", (s+i));          
        }
        
        printf("s[0]=%p\n", &s[0]);
        printf("s[1]=%p\n", &s[1]);

        printf("=================\n");
        printf("p[0]=%p\n", p);
        printf("p[0]=%p\n", p+sizeof(void*));

        for(int i=0;i<3;i++){
            printf("%s\n", *(int*)p);
            p+=sizeof(char *);
        }
    }

    return 0;
}
