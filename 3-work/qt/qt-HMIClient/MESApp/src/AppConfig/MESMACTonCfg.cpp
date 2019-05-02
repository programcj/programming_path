/*
 * MESMachineTon.cpp
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#include "MESMACTonCfg.h"



bool MESMACTonCfg::write(const QString &path)
{
    return false;
}

bool MESMACTonCfg::read(const QString &path)
{
    //char version[6]; //	版本号：6字节的生成日期(HEX)
    quint16 macSize; //	机器总数：2字节（n）
    QFile fileRead(path);

    this->TonList.clear();
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream.readRawData((char*) &macSize, sizeof(macSize));

        this->macSize = macSize;

        for (int i = 0; i < macSize; i++)
        {
            quint16 index; //序号：2字节
            char number[20]; //机器编号：20字节
            quint32 ton; //吨位值：4字节
            char ipAddress[16]; //15
            char machinoBrand[30];
//			序号：2字节
//			机器编号：20字节
//			吨位值：4字节
//			IP地址：15字节
//			机器品牌：30字节
            memset(ipAddress, 0, 16);
            readDataStream.readRawData((char*) &index, sizeof(index));
            readDataStream.readRawData(number, sizeof(number));
            readDataStream.readRawData((char*) &ton, sizeof(ton));
            readDataStream.readRawData(ipAddress, 15);
            readDataStream.readRawData(machinoBrand, sizeof(machinoBrand));

            this->TonList.append(
                    MESMACTonCfg::TonInfo(index, number, ipAddress,
                            machinoBrand, ton));
        }
        fileRead.close();
        return true;
    }
    return false;
}
