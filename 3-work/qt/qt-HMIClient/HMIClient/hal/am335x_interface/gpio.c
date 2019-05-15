#include "interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

//    echo 100 > /sys/class/gpio/export
//    echo 114 > /sys/class/gpio/export
//    echo 103 > /sys/class/gpio/export
//    echo 104 > /sys/class/gpio/export
//    echo 113 > /sys/class/gpio/export
//    echo 111 > /sys/class/gpio/export
//    echo 116 > /sys/class/gpio/export
//    echo 117 > /sys/class/gpio/export
//    echo out > /sys/class/gpio/gpio100/direction
//    echo out > /sys/class/gpio/gpio114/direction
//    echo out > /sys/class/gpio/gpio103/direction
//    echo out > /sys/class/gpio/gpio104/direction
//    cat /sys/class/gpio/gpio113/value
//
//    echo 113 /sys/class/gpio/unexport
// ----------------------------------------------------
//   100 114 103 104
// output : GPIO3_4 GPIO3_18 GPIO3_7 GPIO3_8
// ----------------------------------------------------
// 113, 111, 116, 117
// GPIO3_17 GPIO3_15 GPIO3_20 GPIO3_21
// ----------------------------------------------------

//echo 1 > /sys/class/gpio/gpio100/value
//echo 1 > /sys/class/gpio/gpio114/value
//echo 1 > /sys/class/gpio/gpio103/value
//echo 1 > /sys/class/gpio/gpio104/value

//echo 0 > /sys/class/gpio/gpio100/value
//echo 0 > /sys/class/gpio/gpio114/value
//echo 0 > /sys/class/gpio/gpio103/value
//echo 0 > /sys/class/gpio/gpio104/value

static const char *GPIO_INPUT[]= { "113", "111", "116", "117"};
static const char *GPIO_OUT[]= {"100", "114", "103", "104"};

//GPIO OUT:
//114, 1 - 0
//103
int file_exists(const char *file)
{
    return access(file,F_OK)!=-1?1:0;
}

void gpio_init_out()
{
#ifdef AM335X
    FILE *fp=NULL;
    int i=0;
    char path[150];
    for(i=0;i<4;i++){
        sprintf(path,"/sys/class/gpio/gpio%s/direction", GPIO_OUT[i]);
        if(file_exists(path)==1)
            continue;

        fp=fopen("/sys/class/gpio/export","w");
        if(fp){
            fprintf(fp,"%s",GPIO_OUT[i]);
            fflush(fp);
            fclose(fp);
        }
        usleep(10*1000); //10ms
        fp=fopen(path,"w");
        if(fp){
            fprintf(fp,"out");
            fflush(fp);
            fclose(fp);
        }
    }
#endif
}

int gpio_set_value(unsigned int outindex,int value)
{
#ifdef AM335X
    char path[150];
    FILE *fp=NULL;
    sprintf(path,"/sys/class/gpio/gpio%s/value",GPIO_OUT[outindex]);
    fp=fopen(path,"w");
    if(fp){
        fprintf(fp,"%d",value);
        fflush(fp);
        fclose(fp);
        return 0;
    }
#endif
    return -1;
}

void gpio_init_input()
{
 #ifdef AM335X
    FILE *fp=NULL;
    int i=0;
    char path[150];
    for(i=0;i<4;i++){
        sprintf(path,"/sys/class/gpio/gpio%s/direction", GPIO_INPUT[i]);
        if(file_exists(path)==1)
            continue;

        fp=fopen("/sys/class/gpio/export","w");
        if(fp){
            fprintf(fp,"%s",GPIO_INPUT[i]);
            fflush(fp);
            fclose(fp);
        }
    }
 #endif
}

int gpio_out_read(int outindex)
{
    int value=0;
#ifdef AM335X
    if(outindex<4) {
        if(0==gpio_read(GPIO_OUT[outindex],&value)){
            return value;
        }
    }
    return -1;
#endif
}

int gpio_read(const char *gpio,int *value)
{
#ifdef AM335X
    char path[150];
    char v[10];
    FILE *fp=NULL;
    sprintf(path,"/sys/class/gpio/gpio%s/value",gpio);

    fp=fopen(path,"r");
    if(fp){
        fseek(fp,0,0);
        memset(v,0,sizeof(v));
        fgets(v,10, fp);
        fclose(fp);

        if(value){
            *value=atoi(v);
        }
        //printf("%s=%s,%d=\n",path,v, *value);
        return 0;
    }
#endif
    return -1;
}

int gpio_input_read(unsigned int inputindex,int *value)
{
    if(inputindex<4)
        return gpio_read(GPIO_INPUT[inputindex],value);
    return -1;
}

const char *gpio_input_name(unsigned int index)
{
    if(index<4)
        return GPIO_INPUT[index];
    return "NULL";
}

void gpio_test()
{
    int i=0;
    int value;
    gpio_init_input();
    gpio_init_out();
    while(1){
        printf("=====================\n");
        for(i=0;i<4;i++) {
            gpio_input_read(i,&value);
            printf("in <-GPIO%d(%s)=%d \n",i, GPIO_INPUT[i], value);
        }
        printf("=====================\n");
        for(i=0;i<4;i++) {
            gpio_set_value(i,1);
            gpio_read(GPIO_OUT[i],&value);
            printf("out->GPIO%d(%s)=1,v=%d \n",i, GPIO_OUT[i], value);
            sleep(1);

            gpio_set_value(i,0);
            gpio_read(GPIO_OUT[i],&value);
            printf("out->GPIO%d(%s)=1,v=%d  \n",i, GPIO_OUT[i], value);
            sleep(1);
            getchar();
        }
    }
}
