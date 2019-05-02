#include "halrfidread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include "am335x_interface/interface.h"

HALRFIDRead::HALRFIDRead(QObject *parent) :
    QThread(parent)
{
#if QT_VERSION==0x040805
    interruptionRequested=false;
#endif

}

HALRFIDRead::~HALRFIDRead()
{

}

#if QT_VERSION==0x040805
void HALRFIDRead::requestInterruption()
{
    QMutexLocker locker(&selfMutex);
    interruptionRequested=true;
}

bool HALRFIDRead::isInterruptionRequested()
{
    QMutexLocker locker(&selfMutex);
    return interruptionRequested;
}
#endif

void HALRFIDRead::run()
{
    /**
     *调用读RFID功能
     */
    QString uid="";

#if QT_VERSION==0x050401
    QThread::sleep(2);
    emit this->sig_ReadRFIDUID("test123456");
#endif

#ifdef Q_OS_LINUX_WEIQIAN
    int len,ret;
    int fd=0;
    char buff[100];
    fd_set rd;
    struct timeval tv;

    fd=open("/dev/ttyAMA0",O_RDWR);
    {
        struct termios options;
        tcgetattr(fd, &options);
        options.c_cflag |= ( CLOCAL | CREAD );/*input mode flag:ignore modem control lines; Enable receiver */
        options.c_cflag &= ~CSIZE;
        //options.c_cflag &= ~CRTSCTS;
        options.c_cflag |= CS8;
        options.c_cflag &= ~CSTOPB; //停止位
        options.c_iflag |= IGNPAR;	// 忽略校验错误
        options.c_oflag &=~OPOST;//raw output mode
        options.c_lflag &=~(ICANON|ECHO|ECHOE|ISIG);//raw input mode
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        tcsetattr(fd,TCSANOW,&options);
        tcflush(fd,TCIFLUSH); //flush input queue
    }

    while(!isInterruptionRequested()) {
        uid="";
        if(fd>0){
            //在这里读取RFID的卡号UID，这里只是我的模拟  ,need  get_card fun....... plase write copy from rfid_test.c
            FD_ZERO(&rd);
            FD_SET(fd,&rd);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            ret=select(fd+1,&rd,NULL,NULL,&tv);
            if(ret>0){
                memset(buff,0,sizeof(buff));
                read(fd,buff,sizeof(buff));
                uid=QString("%1").arg(buff);
            } else {

            }
        }
        if(uid.length()>0)
            emit this->sig_ReadRFIDUID(uid);
    }
    if(fd>=0)
        close(fd);
#endif


#ifdef AM335X
    int fd=0;
    int i=0;
    int len;

    unsigned char arrayBeep[]={0xaa,0xbb,0x06,0x00,0x00,0x00,0x06,0x01,0x01,0x06}; //Beep
    unsigned char arrayFindCard[]={0xaa,0xbb,0x06,0x00,0x00,0x00,0x01,0x02,0x52,0x51}; //find_card
    unsigned char arrayAnti[]={0xaa,0xbb,0x06,0x00,0x00,0x00,0x02,0x02,0x04,0x04}; //anti_collision

    unsigned char recv_data[50];
    unsigned char check_sum=0;
    quint32 iccardid=0;
    int check_sum_position;

    fd=open(AM335X_COM_RFID,O_RDWR);
    {
        struct termios options;
        tcgetattr(fd, &options);
        options.c_cflag |= ( CLOCAL | CREAD );/*input mode flag:ignore modem control lines; Enable receiver */
        options.c_cflag &= ~CSIZE;
        //options.c_cflag &= ~CRTSCTS;
        options.c_cflag |= CS8;
        options.c_cflag &= ~CSTOPB; //停止位
        options.c_iflag |= IGNPAR;	// 忽略校验错误
        options.c_oflag &=~OPOST;//raw output mode
        options.c_lflag &=~(ICANON|ECHO|ECHOE|ISIG);//raw input mode
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        tcsetattr(fd,TCSANOW,&options);
        tcflush(fd,TCIFLUSH); //flush input queue
    }

    while(!isInterruptionRequested()) {
        uid="";

        if(fd>0){
            tcflush(fd,TCIFLUSH); //flush input queue
            write(fd,arrayFindCard,sizeof(arrayFindCard));
            usleep(200 * 1000); //2000 000=2s
            memset(recv_data,0,sizeof(recv_data));
            len=read(fd,recv_data,sizeof(recv_data));
            //            printf("---------%d\r\n",len);
            //            for(i=0;i<len;i++){
            //                printf("%02X ",recv_data[i]);
            //            }
            //            printf("\n");

            tcflush(fd,TCIFLUSH); //flush input queue
            write(fd,arrayAnti,sizeof(arrayAnti));
            usleep(200 * 1000);
            memset(recv_data,0,sizeof(recv_data));
            len=read(fd,recv_data,sizeof(recv_data));

            //            for(i=0;i<len;i++){
            //                printf("%02X ",recv_data[i]);
            //            }
            //            printf("\n");
            if(len>=14){
                iccardid=0;
                check_sum=0;

                for(i=0; i<recv_data[2]-1; i++){
                    check_sum ^= recv_data[4+i];
                }
                check_sum_position = 4 + recv_data[2] -1;

                printf("check_sum=%02X,check_sum_position=%02X\r\n",check_sum,check_sum_position);

                if(check_sum == recv_data[check_sum_position] && recv_data[2]==0x0A){
                    memcpy(&iccardid,recv_data+3+10-4,4);

                    printf("iccardid=%04X,%d\r\n",iccardid,iccardid); //1630146541

                    uid=QString("%1").arg(iccardid);
                    qDebug() << "RFID:" <<uid;
                    tcflush(fd,TCIFLUSH); //flush input queue
                    write(fd,arrayBeep,sizeof(arrayBeep));
                    usleep(200 * 1000); //2000 000=2s
                }
                //AA BB 0A 00 00 00 02 02 00 B2 1E B3 69 76
                //AA BB 0A 00 00 00 02 02 00 ED 0F 2A 61 A9

                //0x612A0FED = 1630146541
            }

            //AA BB [len] [....] [check]
        }

        if(uid.length()>0) {
            qDebug() <<"emit sig_ReadRFIDUID:"<<uid;
            emit sig_ReadRFIDUID(uid);
            break;
        }
        QThread::sleep(1);
    }

    if(fd>=0)
        close(fd);
#endif

    qDebug() << QString("HALRFIDRead--exit:%1").arg(uid);
}
