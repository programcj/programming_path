#include "serial.h"

#include <QDebug>

#define RS485_SEND "echo 1 > /sys/class/misc/io_ctl/gpio_state"
#define RS485_RECEIVE "echo 0 > /sys/class/misc/io_ctl/gpio_state"

Serial::Serial(const char* port)
		:fd(0)
{
#ifndef _WIN32
     if(strcmp(port, "/dev/ttyO1")==0)
     {
		system(RS485_RECEIVE);     //控制GPIO0_13引脚电平使485允许接收,禁止发送。
		workmode = RS485Mode;
		qDebug()<<"485 mode";
		fd = open( port, O_RDWR | O_NOCTTY | O_NDELAY);
     }
     else
     {
        workmode = RS232Mode;
        qDebug()<<"232 mode";
        fd = open( port, O_RDWR);
     }

 // fd = open( port, O_RDWR | O_NOCTTY | O_NDELAY);         //| O_NOCTTY | O_NDELAY

  if (-1 == fd)
  {
      /*设置数据位数*/
      qDebug()<<"Can't Open Serial Port";
  }
  qDebug()<<"Serial fd is" << fd;



#endif

}

Serial::~Serial()
{
#ifndef _WIN32
    close(fd);
#endif
}
// 初始化
void Serial::init(int Biterate,int databits,int stopbits,int parity)
{
#ifndef _WIN32
	set_Biterate(Biterate);
   if(set_Parity(databits,stopbits,parity)== FALSE)
   {
	qDebug()<< "Set Parity Error";
	close(fd);
	exit(1);
   }
#endif
}


//设置参数
int Serial::set_Parity(int databits,int stopbits,int parity)
{
#ifndef _WIN32
	struct termios options;
	if (tcgetattr( fd,&options)  !=  0)
	{
	    qDebug()<<"SetupSerial 1";
	    return false;
	}
	options.c_cflag &= ~CSIZE ;
	options.c_oflag = 0;
	switch (databits) /*设置数据位数*/
	{
	  case 7:
		  options.c_cflag |= CS7;
		  break;
	  case 8:
		  options.c_cflag |= CS8;
		  break;
	  default:
		  qDebug()<<"Unsupported data size\n";
		  return (FALSE);
	}

	switch (parity)
	{
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		qDebug()<<stderr,"Unsupported parity\n";
		return false;
	}
	/* 设置停止位*/
	switch (stopbits)
	{
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		qDebug()<<"Unsupported stop bits\n";
		return false;
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;

	options.c_cc[VTIME] = 0; // 1 seconds 字符之间的时间
	options.c_cc[VMIN] = 0;

	options.c_lflag &= ~(ECHO | ICANON);
//	options.c_cflag |= CRTSCTS;


	tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		qDebug()<<"SetupSerial 3";
		return false;
	}

#endif
	return true;

}


// 发送数据
unsigned int Serial::sendData(const char* data, unsigned int maxSize)
{
  unsigned int sendlen = 0;
  /*for(int i=0; i<maxSize;i++ )
    {
      qDebug("%2x\n",data[i]);
    }*/
#ifndef _WIN32
  if(workmode == RS485Mode)
  {
      system(RS485_SEND);   //使能RTS，发送数据
      usleep(100);
      sendlen = write(fd,data, maxSize);
      system(RS485_RECEIVE); //清零RTS，接收数据
  }
  else
  {
      sendlen = write(fd,data, maxSize);
  }
#endif
  return sendlen;

}

// 读取数据
unsigned int Serial::readData(char* data, unsigned int maxSize)
{

#ifndef _WIN32
    return read(fd,data, maxSize);
#endif
}

int Serial:: getFd()
{
  return fd;
}

void Serial::set_Biterate(int biteratetype)
{
	int   i;
	int   status;
#ifndef _WIN32
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    cfmakeraw(&Opt);
    tcflush(fd, TCIOFLUSH);

	switch(biteratetype)
	{
	case B_9600_BITERATE: //9600
		 cfsetispeed(&Opt, B9600);
		 cfsetospeed(&Opt, B9600);
		 break;
	case B_115200_BITERATE: //115200
		 cfsetispeed(&Opt, B115200);
		 cfsetospeed(&Opt, B115200);
		break;
	case B_1200_BITERATE:
		cfsetispeed(&Opt, B1200);
		cfsetospeed(&Opt, B1200);
		break;

	default:
		 cfsetispeed(&Opt, B9600);
		 cfsetospeed(&Opt, B9600);

	}
	 status = tcsetattr(fd, TCSANOW, &Opt);
	if (status != 0)
	  qDebug()<<"tcsetattr fd1";
#endif
}
