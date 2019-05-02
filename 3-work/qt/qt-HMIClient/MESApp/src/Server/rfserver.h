/*
 * rfserver.h
 *
 *  Created on: 2015年3月19日
 *      Author: cj
 */

#ifndef RFSERVER_H_
#define RFSERVER_H_
#include <QObject>
#include <QString>

#include "../HAL/rfidlister.h"

#define RFID_DATA_LEN  9

class RFIDServer: public QObject
{
  Q_OBJECT
public:
  RFIDServer ();
  virtual
  ~RFIDServer ();

  static RFIDServer *GetInstance();
  void RFIDStart();
  void RFIDEnd();

private:
  RFIDLister rfLister; //射频卡侦听
  char orgdata[10];
  int  orglen;
  char RFNum[20];
 // static RFIDServer Instance;
 //QString  RFNum;
  void getcardno(char* cardno);

 signals:
      void RFIDData(char* databuff);

 public slots:
     void  ondataReceive(char* data,int len);
};

#endif /* RFSERVER_H_ */
