/*
 * MESFaultTypeCfg.cpp
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#include "MESFaultTypeCfg.h"

void MESFaultTypeCfg::Debug() {
//	logDebug("故障总数:%d", sumSize);
//	for (int i = 0; i < FaultInfoList.size(); i++) {
//		logDebug("故障名称:%s", FaultInfoList[i].getTypeName().toAscii().data());
//		logDebug("故障类型数量:%d", FaultInfoList[i].getTypeSize());
//		for (int j = 0; j < FaultInfoList[i].getTypeList().size(); j++) {
//			logDebug("%d:%s", FaultInfoList[i].getTypeList()[j].Index,
//					FaultInfoList[i].getTypeList()[j].Type.toAscii().data());
//		}
    //	}
}

bool MESFaultTypeCfg::write(const QString &path)
{
    return false;
}


bool MESFaultTypeCfg::read(const QString &path)
{
    //char version[6]; //	版本号：6字节的生成日期(HEX)
    quint8 faultSize; //故障总数：1字节（n）

    this->FaultInfoList.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream.readRawData((char*) &faultSize, sizeof(faultSize));

        this->sumSize = faultSize;

        for (int i = 0; i < faultSize; i++)
        {
            char faultName[30]; //故障名称：机器故障：30字节
            quint8 faultTypeSize; //故障类型数量：1字节（M1）
            readDataStream.readRawData(faultName, sizeof(faultName));
            readDataStream.readRawData((char*) &faultTypeSize,
                    sizeof(faultTypeSize));

            MESFaultTypeCfg::TypeInfo faultItem;
            faultItem.setTypeName(faultName);
            faultItem.setTypeSize(faultSize);
            for (int j = 0; j < faultTypeSize; j++)
            {
                quint8 faultIndex; //第1个故障序号：1字节
                char faultType[30]; //第1个故障类型：30字节
                readDataStream.readRawData((char*) &faultIndex,
                        sizeof(faultIndex));
                readDataStream.readRawData(faultType, sizeof(faultType));

                faultItem.getTypeList().append(
                        MESFaultTypeCfg::TypeItem(faultIndex, faultType));
            }
            this->FaultInfoList.append(faultItem);
        }
        fileRead.close();
        return true;
    }
    return false;
}
