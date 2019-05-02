/*
 * rfserver.cpp
 *
 *  Created on: 2015年3月19日
 *      Author: cj
 */

#include "rfserver.h"

#include <stdio.h>
#include <QtDebug>
#include "../Public/public.h"
//RFIDServer RFIDServer:: Instance;

RFIDServer::RFIDServer ()
    :orglen(0)
{
  // TODO Auto-generated constructor stub
  connect(&rfLister,SIGNAL(serialdataReady(char*,int)),this,SLOT(ondataReceive(char*,int)));

}

RFIDServer::~RFIDServer ()
{
  // TODO Auto-generated destructor stub
  //delete rfLister;
}

RFIDServer *RFIDServer::GetInstance()
{
  static RFIDServer* Instance = 0;
      if (0 == Instance)
      {
	  Instance = new RFIDServer();
      }
  return Instance;
}
void RFIDServer::RFIDStart()
{
  rfLister.ClearRevbuff();
  rfLister.start();
}
void RFIDServer::RFIDEnd()
{
  rfLister.RFIDStop();
 // rfLister.exit();
}
void RFIDServer::ondataReceive(char* data,int len)
{
  if(len >= RFID_DATA_LEN)
  {
      memcpy(orgdata,data,len);
      memset(RFNum,0,sizeof(RFNum));
      int datalen = orgdata[2];
      for(int i= 0; i < datalen; i++)
      {
         sprintf(&RFNum[i*2],"%02X",orgdata[3+i]);
      }
      for(int i=0; i< datalen*2; i++)
      {
           qDebug()<<RFNum[i];
      }
     Tool::ExeBuzzer();
      emit RFIDData(RFNum);  //前面两个字节没有使用
  }
}

void RFIDServer::getcardno(char* cardno)
{
   memset(cardno,0,9);
   memcpy(cardno,RFNum,sizeof(RFNum));
}
