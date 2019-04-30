#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    struct winsize siz;
    int ret = 0;
    int fd = 0;
   
    ret = ioctl(stderr, TIOCGWINSZ, &siz);
    if (ret < 0)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        fd = fileno(stderr);
        ret = ioctl(fd, TIOCGWINSZ, &siz);
    }
    
    printf("fileno=%d\n", fd);
    printf("ret=%d\n", ret);
    printf("ws_col=%d\n", siz.ws_col);
    printf("row=%d\n", siz.ws_row);
    return 0;
}
