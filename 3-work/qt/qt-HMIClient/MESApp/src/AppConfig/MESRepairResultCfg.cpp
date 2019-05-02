/*
 * MESRepairResultCfg.cpp
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#include "MESRepairResultCfg.h"



bool MESRepairResultCfg::write(const QString &path)
{
    return false;
}

bool MESRepairResultCfg::read(const QString &path)
{
    //char version[6]; //版本号：6字节的生成日期(HEX)
    quint8 repairSize; //故障总数：1字节（n）

    this->FaultInfoList.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream.readRawData((char*) &repairSize, sizeof(repairSize));

        this->sumSize = repairSize;

        for (int i = 0; i < repairSize; i++)
        {
            char faultName[30]; //维修名称：机器故障：30字节
            quint8 repairSize; //维修结果数量：1字节（M1）
            readDataStream.readRawData(faultName, sizeof(faultName));
            readDataStream.readRawData((char*) &repairSize, sizeof(repairSize));

            MESFaultTypeCfg::TypeInfo faultItem;
            faultItem.setTypeName(faultName);
            faultItem.setTypeSize(repairSize);

            for (int j = 0; j < repairSize; j++)
            {
                quint8 Index; //第1个结果序号：1字节
                char Type[30]; //第1个结果类型：30字节
                readDataStream.readRawData((char*) &Index, sizeof(Index));
                readDataStream.readRawData(Type, sizeof(Type));

                faultItem.getTypeList().append(
                        MESFaultTypeCfg::TypeItem(Index, Type));
            }
            this->FaultInfoList.append(faultItem);
        }
        fileRead.close();
        return true;
    }
    return false;
}
