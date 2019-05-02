/*
 * collection.h
 *
 *  Created on: 2015年2月11日
 *      Author: cj
 */

#ifndef COLLECTION_H_
#define COLLECTION_H_

#include <QObject>
#include <QMutex>
#include <QtGlobal>
#include "../Public/public.h"
#include "../Public/emnu.h"
#include "../HAL/rs485lister.h"

//温度
struct  temperature
{
    quint16 tempbuff[1024];
    quint16 count;
    quint16 maxtemp;
    quint16 mintemp;
    quint16 lasttemp;
    quint32 sum;
};

class Collection:public QObject
{
    Q_OBJECT
public:
    Collection ();
    virtual  ~Collection ();

    void ColetctStart();
    void ColetctStop();

    bool getModuleChannleValue(quint8 channlenum,quint16 &value);   //12个IO 口数据

    static Collection *GetInstance();

    QList<int> read_temperature_value(int tempNum);
    QList<int> read_pressure_value(int pointNum);
    bool setTempCalibre(int channel,int value);

private:
    quint8 revdata[MAX_MODBUS_FRAME_LEN];
    quint8 devAddr;
    quint8 funNo;
    quint16 uwDataLen;
    void recvDump(const char* dataBuffer, int dataLen);
    void parsedata(char *orgdata, int len);
    quint16 uwIoModuleData[IO_MODULE_CHANNEL_NUM];
    QMutex mutex;
    // static Collection Instance;
    QList<int> pressureList;

    temperature tempqueue[8];

public slots:
    void  onDataProtocol(char* data,int len);

private:
    Rs485Lister rs485lister;  //监听线程
};

#endif /* COLLECTION_H_ */
