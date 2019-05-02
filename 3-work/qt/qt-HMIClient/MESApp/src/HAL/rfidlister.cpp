/*
 * seriallister.cpp
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */

#include <QtDebug>

#include "rfidlister.h"
#include <QFile>
#ifndef _WIN32
#include <fcntl.h>
#endif
#define max_recv_buffer  (4*1024)


RFIDLister::RFIDLister()
{
#ifndef _WIN32
  serial = new Serial("/dev/ttyO2");
  serial->init(B_9600_BITERATE,8,1,'N');
#endif

}

RFIDLister::~RFIDLister ()
{
  // TODO Auto-generated destructor stub
#ifndef _WIN32
  delete serial;
#endif
}
quint16 RFIDLister::sendDataToComm(char* data,quint16 len)
{
  return serial->sendData(data,len);
}

static int send_counter = 0;
void RFIDLister::sendvDump(const char* dataBuffer, int dataLen)
{
  send_counter ++;

    QString filename = "dat/send/" + QString::number(send_counter);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(dataBuffer, dataLen);
        file.close();
    }
}

void RFIDLister::RFIDStop()
{
  rfidstate = false;
}

void RFIDLister::ClearRevbuff()
{

#ifndef _WIN32
   tcflush(serial->getFd(), TCIFLUSH);
#endif

}
void RFIDLister::run()
{
  char recvBuffer [max_recv_buffer];
  quint16 recvlen = 0;
  memset(recvBuffer, 0, max_recv_buffer);
  rfidstate = true;
  while (rfidstate)
  {
#ifndef _WIN32
	  struct timeval tv;
	  tv.tv_sec = 5;
	  tv.tv_usec = 0;

	  fd_set fdset;
	  FD_ZERO(&fdset);


	  int fd = serial->getFd();
	 // qDebug() << "serial fd is "<< fd;
	  FD_SET(fd, &fdset);

          int ret = select(fd + 1, &fdset, NULL, NULL, &tv);
          if (0 < ret)
          {
              if (FD_ISSET(fd, &fdset))
              {
                  int len = serial->readData(&recvBuffer[recvlen], max_recv_buffer);
                  if (0 < len)
                  {
                      recvlen +=len;
                     /* qDebug("receive data from RFID %2x,%2x,%2x",recvBuffer[0] , recvBuffer[1] , recvBuffer[recvlen - 1]);
                      for(int i=0;i<recvlen;i++)
                       {
                	  qDebug("rfid[%d]: %02x \n",i,recvBuffer[i]);
                       }*/
                      // 收到数据信号
                      if( recvBuffer[0]==0xaa && recvBuffer[1]==0xbb && recvBuffer[recvlen - 1] == 0xef)
                       {
						  emit serialdataReady(recvBuffer,recvlen);
						  recvlen = 0;
                       }
                  }
              }
          }
//          else if (0 == ret)
//          {
//              qDebug() << "RFID monito  select timeout ";
//          }
//          else
//          {
//              qDebug() << "RFID monitor select fail ";
//          }

#endif
          QThread::msleep(500);
      }
}

