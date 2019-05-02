/*
 * collection.cpp
 *
 *  Created on: 2015年2月11日
 *      Author: cj
 */

#include "collection.h"
#include <QDebug>
#include "../HAL/rs485lister.h"

//Collection Collection:: Instance;

Collection::Collection() :
    devAddr(1), funNo(3), uwDataLen(0)
{
    // TODO Auto-generated constructor stub
    // rs485lister = new Rs485Lister();
    connect(&rs485lister, SIGNAL(serialdataReady(char*,int)), this,
            SLOT(onDataProtocol(char*,int)));
    memset(&tempqueue,0,sizeof(tempqueue));
}

Collection::~Collection()
{
    // TODO Auto-generated destructor stub
    //delete rs485lister;
}

void Collection::ColetctStart()
{
    //rs485lister.setPriority(QThread::HighPriority);
    rs485lister.start(); //485监听
}

void Collection::ColetctStop()
{
    rs485lister.Rs485Stop();
    //  rs485lister.exit(); //485监听
}

static int recv_counter = 0;
//????
void Collection::recvDump(const char* dataBuffer, int dataLen)
{
    recv_counter++;

    QString filename = "dat/recv/" + QString::number(recv_counter);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(dataBuffer, dataLen);
        file.close();
    }
}

void Collection::onDataProtocol(char* data, int len)
{
    // qDebug()<<"receive:"<<len;
    quint16 uwCalcCheckSum = 0;
    quint16 uwGetCalcCheckSum = 0;
    quint16 datalen = 0;
    quint8 tempdata[1024];

    if ((data[0] == devAddr) && (data[1] == funNo))
    {
        memcpy(revdata, data, len);
        uwDataLen = len;
    } else if (uwDataLen >= 0 && uwDataLen < MAX_MODBUS_FRAME_LEN) {
        memcpy(revdata + uwDataLen, data, len);
        uwDataLen += len;
    } else {
        qDebug() << "iomodule data error!" << "LEN:" << len << "uwDataLen:"
                 << uwDataLen;
        uwDataLen = 0;
    }

    while (1)
    {
        for (int i = 0; i < uwDataLen; i++)
        {
            if (revdata[i] == devAddr && revdata[i + 1] == funNo)
            {
                if (i > 0)
                    memcpy(revdata, revdata + i, uwDataLen - i);
                uwDataLen -= i;
                if (uwDataLen > 2)
                    datalen = revdata[2];  	       //数据的长度
                break;
            }
        }
        if (uwDataLen <= datalen || datalen == 0)
        {
            break;
        }
        uwCalcCheckSum = rs485lister.crc16(revdata, datalen + 3);  //计算checksum值
        uwGetCalcCheckSum = (revdata[datalen + 3 + 1] << 8)
                + revdata[datalen + 3 + 0]; //取checksum 的值
        if (uwCalcCheckSum == uwGetCalcCheckSum)
        {
            memcpy(tempdata, (revdata + 3), datalen);
            //mutex.lock();
            parsedata((char*) tempdata, datalen);
            //  mutex.unlock();
            // recvDump((char*)uwIoModuleData,datalen);
            if (uwDataLen > datalen + 5)
            {
                uwDataLen = uwDataLen - datalen - 5;
                memcpy(revdata, (revdata + datalen + 5), uwDataLen);
            }
            else
            {
                uwDataLen = 0;
                break;
            }

            if (uwDataLen < 29)
            {
                break;
            }
        }
        else
        {
            qDebug() << "checksum error!" << "data length:" << uwDataLen;
            break;
        }
    }
}

void Collection::parsedata(char *orgdata, int len)
{
    quint16 udwFlag = 0x0F80;
    for (int i = 0; i < IO_MODULE_CHANNEL_NUM; i++)
    {
        uwIoModuleData[i] = (orgdata[i * 2 + 0] << 8) + orgdata[i * 2 + 1];
        if (i < 4)
        {
            if ((uwIoModuleData[i] & udwFlag) == udwFlag)
                uwIoModuleData[i] = 1;
            else
                uwIoModuleData[i] = 0;
            //  qDebug("ModuleData:%x", uwIoModuleData[i]);
        }
        else if (i == IO_MODULE_VOLTAGE_1 || i == IO_MODULE_VOLTAGE_2)
        {
            pressureList.append(uwIoModuleData[i]);
        }
        else if(i >= IO_MODULE_TEMPER_0 && i<= IO_MODULE_TEMPER_3 )
        {
            int j =i - IO_MODULE_TEMPER_0;
            tempqueue[j].tempbuff[ tempqueue[j].count++] = uwIoModuleData[i];
            if(uwIoModuleData[i] >	tempqueue[j].maxtemp)
                tempqueue[j].maxtemp = uwIoModuleData[i];
            if(uwIoModuleData[i] <	tempqueue[j].mintemp)
                tempqueue[j].mintemp = uwIoModuleData[i];
            tempqueue[j].sum +=uwIoModuleData[i];

            //qDebug("tempvaule[%d]: %d",i, uwIoModuleData[i]);
        }
        // qDebug("ModuleData[%d]:%4x",i, uwIoModuleData[i]);
    }
}

bool Collection::getModuleChannleValue(quint8 channlenum, quint16 &value)
{
    if (channlenum < IO_MODULE_CHANNEL_NUM)
    {   mutex.lock();
        value = uwIoModuleData[channlenum];
        mutex.unlock();
        return true;
    }
    return false;
}

Collection *Collection::GetInstance()
{
    static Collection* Instance = 0;
    if (0 == Instance)
    {
        Instance = new Collection();
    }
    return Instance;
}

QList<int> Collection::read_temperature_value(int tempNum)
{
#define DEFALUT_TEMPERATAUTE_VALUE 1000
    QList<int> tp_data;
    for (int i = 0; i < tempNum; i++)
    {
        if (i < 4)
        {
            if(tempqueue[i].count > 10)
            {
                tempqueue[i].lasttemp = (tempqueue[i].sum - (tempqueue[i].maxtemp + tempqueue[i].mintemp))/(tempqueue[i].count-2);
                tempqueue[i].count = 0;
                tempqueue[i].sum = 0;
                tempqueue[i].maxtemp = tempqueue[i].lasttemp;
                tempqueue[i].mintemp = tempqueue[i].lasttemp;

            }
            tp_data.append(tempqueue[i].lasttemp);
            //			qDebug() << "IO_MODULE_TEMPER_channel" << IO_MODULE_TEMPER_0 + i
            //					<< tempqueue[i].lasttemp;

        }
        else
        {
            tp_data.append(DEFALUT_TEMPERATAUTE_VALUE);
        }
    }

    return tp_data;
}

QList<int> Collection::read_pressure_value(int pointNum)
{
    QList<int> tp_data;
    tp_data = pressureList.mid(0,
                               pressureList.size() > pointNum ?
                                   pointNum - 1 : pressureList.size());
    if (pressureList.size() > pointNum)
    {
        pressureList = pressureList.mid(pointNum, pressureList.size());
    }
    else
    {
        pressureList.clear();
    }
    return tp_data;
}

bool Collection::setTempCalibre(int channel, int value)
{
    return rs485lister.SetTempCalibration(channel, value);
}
