/*
 * seriallister.h
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */

#ifndef RFIDLISTER_H_
#define RFIDLISTER_H_
#include <QThread>
#include "serial/serial.h"


class RFIDLister: public QThread
{
  Q_OBJECT
public:
  RFIDLister ();

  virtual
  ~RFIDLister ();

 quint16 sendDataToComm(char* data,quint16 len);

// quint16 SendCmdToComm(quint8 ucDevAddr, quint8 ucFunNo, quint16 uwStartAddr, quint16 uwData);

// quint16 crc16(quint8 *pucBuf, quint16 uwLen);

 //保存发送数据到文件
 void sendvDump(const char* dataBuffer, int dataLen);
 void RFIDStop();
 void ClearRevbuff();
 bool rfidstate;

signals:
      // 收到数据信号
     // void dataReady(char* dataBuffer, int dataLen);
      void serialdataReady(char* dataBuffer, int dataLen);

  protected:
      // 线程执行体
      virtual void run();
  private:
      Serial* serial;
};

#endif /* SERIALLISTER_H_ */
