#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h" 

//const char *leds_name[15]={"led0","led1","led2","led3","wifi_run","wifi_cs","wifi_en",NULL};
static const char *leds_name[5]={
    "led0","led1","led2","led3","wifi_run"};

int led_set(int index,int value)
{
    char vstr[10];
    char absolute_path[100];
    int fd=0;
    if(index<0 || index>4)
        return -1;

    sprintf(absolute_path,"/sys/class/leds/%s/brightness",leds_name[index]);

    fd=open(absolute_path,O_RDWR);
    if(fd>0){
        lseek(fd,0,SEEK_SET);
        sprintf(vstr,"%d",value);
        write(fd,vstr,strlen(vstr));
        close(fd);
        return 0;
    }
    return -1;
}

int led_value(int index)
{
    char vstr[10];
    char absolute_path[100];
    int fd=0;
    if(index<0 || index>4)
        return -1;
    sprintf(absolute_path,"/sys/class/leds/%s/brightness",leds_name[index]);

    fd=open(absolute_path,O_RDWR);
    if(fd>0){
        lseek(fd,0,SEEK_SET);
        memset(vstr,0,sizeof(vstr));
        read(fd,vstr,sizeof(vstr));
        close(fd);
    }
    return atoi(vstr);
}

/*
 * @path : the path of device file corresponding to LED
 */
//int led_set(char *path,enum led_value value)
//{/*{{{*/
//	int fd;
//	unsigned char led_set_value = (unsigned char)value;
//	unsigned char led_set_status;
//	int ret;
	
//	fd = open(path,O_RDWR);
//	if(fd < 0){
//		printf("open %s -->",path);
//		err_msg("open():");
//	}

//	ret = write(fd,&led_set_value,sizeof(led_set_value));
//	if(ret != sizeof(led_set_value)){
//		err_msg("write():");
//	}
	
//	lseek(fd,0,SEEK_SET);

//	ret = read(fd,&led_set_status,sizeof(unsigned char));
//	if( ret < sizeof(unsigned char)){
//		err_msg("read():");
//	}
	
//	if(led_set_status != led_set_value){
//		if(led_set_value){
//			printf("led_set on:%s failed!\n",path);
//		}else{
//			printf("led_set off:%s failed!\n",path);
//		}
//		return -1;
//	}
	
//	if( close(fd) < 0){
//		err_msg("close():");
//	}
//	return 0;
//}/*}}}*/
/*
 * @num; the serial num of gpio . Range from 0 to 4.
 * 		0-3  corresponding to DIO - DIO3 
 * 		4 	 corresponding to wifi_run 
 * @return value : 0 --> success , 1 --> failed  
 */
//int led_on(unsigned int num)
//{/*{{{*/
//	char absolute_path[PATH_LEN];
//	sprintf(absolute_path,"/sys/class/leds/%s/brightness",leds_name[num]);
//	if( led_set(absolute_path,LEDON) < 0){
//		err_msg("led_on():");
//	}else{
//		printf("led on %s success!\n",absolute_path);
//	}
//	return 0;
//}/*}}}*/
//int led_off(unsigned int num )
//{/*{{{*/
//	char absolute_path[PATH_LEN];
//	sprintf(absolute_path,"/sys/class/leds/%s/brightness",leds_name[num]);
//	if( led_set(absolute_path,LEDOFF) < 0){
//		err_msg("led_off():");
//	}else{
//		printf("led off %s success!\n",absolute_path);
//	}
//	return 0;
//}/*}}}*/
//void  test_all_led()
//{/*{{{*/
//	int i;
//	for(i=0; leds_name[i]!=NULL; i++){
//		led_on(i);
//		sleep(1);
//		led_off(i);
//		sleep(1);
//	}
//}/*}}}*/
