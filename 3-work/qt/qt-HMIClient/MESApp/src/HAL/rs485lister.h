/*
 * seriallister.h
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */

#ifndef RS485LISTER_H_
#define RS485LISTER_H_
#include <QThread>
#include "serial/serial.h"
#include <QMutex>
#include "../Public/iomodule.h"
#include "digitalmeterdl64597.h"



class Rs485Lister: public QThread
{
  Q_OBJECT
public:

  Rs485Lister ();

  virtual
  ~Rs485Lister ();

 quint16 sendDataToComm(char* data,quint16 len);

 quint16 SendCmdToComm(quint8 ucDevAddr, quint8 ucFunNo, quint16 uwStartAddr, quint16 uwData);

 quint16 crc16(quint8 *pucBuf, quint16 uwLen);

 //保存发送数据到文件
 void sendvDump(const char* dataBuffer, int dataLen);
 bool SetTempCalibration(quint8 chinnel, qint16 value );
 //设置485响应时间
 bool set_answer_delay(quint16 puwData);

 void Rs485Stop();
 bool runstate;
 QMutex mutex;


signals:
      // 收到数据信号
     // void dataReady(char* dataBuffer, int dataLen);
      void serialdataReady(char* dataBuffer, int dataLen);

  protected:
      // 线程执行体
      virtual void run();
  private:
      Serial* serial;
      DigitalMeter_Dl64597 *DigitalMeter;
};

#endif /* SERIALLISTER_H_ */
