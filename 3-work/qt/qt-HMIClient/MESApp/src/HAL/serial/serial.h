#ifndef _SERIAL_H_
#define _SERIAL_H_

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <termios.h>    /*PPSIX终端控制定义*/
#endif
//#include <QString>

#define  RS485Mode   1
#define  RS232Mode   0

#define B_9600_BITERATE      0
#define B_115200_BITERATE    1
#define B_1200_BITERATE      2
// 串口通信
class Serial
{

public:
    Serial(const char* port);
    virtual ~Serial();

    // 获取文件句柄
     int getFd();
    // 发送数据
     unsigned int sendData(const char* data, unsigned int maxSize);

    // 读取数据
     unsigned int readData(char* data, unsigned int maxSize);

     void init(int Biterate,int databits,int stopbits,int parity);

private:
    int set_Parity(int databits,int stopbits,int parity);
//    void set_speed_115200bt();
//    void set_speed_9600bt();
    void  set_Biterate(int biteratetype);
	// 设置通信设备

    int workmode;

    int fd;
};


#endif // _SERIAL_H_
