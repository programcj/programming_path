#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

static volatile int waitSeconds=0;

#define LOG(format,...) do { \
     printf(format, ##__VA_ARGS__); \
     fflush(stdout);   \
    } while(0);

void signal_handler(int sig)
{
    switch(sig){
    case SIGUSR1: {
        LOG("signal SIGUSR1");
        waitSeconds=0;
    }
        break;
    }
}

void writeMyPID(){
    register FILE *fp=fopen("/tmp/hmiservice.pid","w");
    if(fp){
        fprintf(fp,"%d",getpid());
        fclose(fp);
    }
}

int main(int argc, char **argv)
{
    LOG("start ,my pid=%d",getpid());

    signal(SIGUSR1,signal_handler);
    writeMyPID();

    waitSeconds=0;

    while(1){
        sleep(1);
        waitSeconds++;
        if(waitSeconds>5){
            LOG("restart program...");
            waitSeconds=0;

            system("killall hmiclient");
            system("/usr/bin/qtLauncher /home/root/hmiclient &");
        }
    }
    return 0;
}
