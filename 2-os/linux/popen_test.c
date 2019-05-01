#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int gdb_readline(int fd, char *buf, int buflen)
{
    int rlen = 0;
    int rsum = 0;
    char *ptr = buf;

    while (buflen)
    {
        rlen = read(fd, buf, 1);
        rsum += rlen;
        //printf("%c", *buf);

        if (*buf == '\r' || *buf == '\n')
        {
            *buf = 0;
            break;
        }
        if (strncmp(ptr, "---Type <return> to continue",
                    strlen("---Type <return> to continue")) == 0)
        {
            break;
        }
        if (*buf == ')' && rsum == 5)
        {
            if (strncmp(ptr, "(gdb)", strlen("(gdb)")) == 0)
            {
                //printf("//////////////(gdb)\n");
                *(buf + 1) == 0;
                break;
            }
        }
        buf++;
        buflen--;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    char buf[1024];
    int ret = 0;

#if 1
    //stdout;
    pid_t chid = 0;
    int sync_pipe_r[2];
    int sync_pipe_w[2];

    pipe(sync_pipe_r);
    pipe(sync_pipe_w);

    if ((chid = fork()) == 0)
    {
        dup2(sync_pipe_w[0], STDIN_FILENO);
        dup2(sync_pipe_r[1], STDOUT_FILENO);

        printf("start [%s]\n", getcwd(NULL, 0));
        fflush(stdout);

        char *earg[] = {"gdb", "dlopen_test", NULL};
        execv("/usr/bin/gdb", earg);
        printf("=======================================....\n");
        fflush(stdout);
        perror("exit...\n");
        return 0;
    }

    signal(SIGCHLD, SIG_IGN);

    printf("chid:%d\n", chid);
    char *cmd = "r\n";
    {
        int flag_r = 0;

        while (1)
        {
            memset(buf, 0, sizeof(buf));
            gdb_readline(sync_pipe_r[0], buf, sizeof(buf));
            if(flag_r==2)
            {
                printf("-%s\n", buf);
            }
            
            if (strstr(buf, "---Type <return> to continue"))
            {
                cmd = "\n";
                write(sync_pipe_w[1], cmd, strlen(cmd));
                printf(">%s\n", cmd);
            }

            if (strncmp(buf, "(gdb)", strlen("(gdb)")) == 0)
            {
                if (flag_r == 0)
                {
                    //cmd = "set confirm off \n";
                    //write(sync_pipe_w[1], cmd, strlen(cmd));
                    //printf(">%s\n", cmd);
                    cmd = "r\n";
                    write(sync_pipe_w[1], cmd, strlen(cmd));
                    printf(">%s\n", cmd);
                    flag_r = 1;
                }
                else
                {
                    if (flag_r == 1)
                    {
                        cmd = "backtrace\r\n";
                        write(sync_pipe_w[1], cmd, strlen(cmd));
                        printf(">%s", cmd);
                        flag_r++;
                    }
                    else
                    {
                        cmd = "quit\n";
                        write(sync_pipe_w[1], cmd, strlen(cmd));
                        printf(">%s", cmd);
                        ret = kill(chid, SIGKILL);
                        printf("KILL %d\n", ret);
                        break;
                    }
                }
            }
            if (strstr(buf, "Quit anyway?"))
            {
                cmd = "y\n";
                write(sync_pipe_w[1], cmd, strlen(cmd));
                printf(">%s", cmd);
                flag_r = 0;
                kill(chid, SIGKILL);
                break;
            }
        }
    }

    exit(1);
#else
    FILE *fp = NULL;

    int fdout;

    int fd = dup(STDOUT_FILENO);

    //STDOUT_FILENO);

    fp = popen("gdb ./dlopen_test", "w");

    if (!fp)
    {
        perror("popen...\r\n");
        return -1;
    }

    if (fp)
    {
        //         FILE *fdopen(int fd, const char *mode);
        // int fileno(FILE *stream);
        while (1)
        {
            memset(buf, 0, sizeof(buf));
            gdb_readline(fd, buf, sizeof(buf));
            //read(sync_pipe_r[0], buf, sizeof(buf));
            //printf("-[%s", buf);
            if (strstr(buf, "Reading"))
            {
                fprintf(fp, "r\r\n");
                fflush(fp);
            }
        }
        pclose(fp);
    }
#endif
    return 0;
}
