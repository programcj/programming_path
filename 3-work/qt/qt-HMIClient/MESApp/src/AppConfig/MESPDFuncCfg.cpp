/*
 * MESOrderFunButton.cpp
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#include "MESPDFuncCfg.h"

MESPDFuncCfg::SetKeyInfo* MESPDFuncCfg::getSetKeyInfo(quint16 funIndex) {
	for (int i = 0; i < SetKeyList.size(); i++) {
		if (SetKeyList[i].funIndex == funIndex)
			return &SetKeyList[i];
	}
	for (int i = 0; i < NotSetKeyList.size(); i++) {
        if (NotSetKeyList[i].funIndex == funIndex)
			return &NotSetKeyList[i];
	}
    return 0;
}

void MESPDFuncCfg::Debug()
{
}


bool MESPDFuncCfg::read(const QString &path)
{
    //char version[6];	//版本号：6字节的生成日期(HEX)
    quint8 notSetSize = 0; //不可配置按键总数：1字节（m）（m目前固定为2）
    quint8 setSize = 0; //可配置按键总数：1字节（n）

    this->NotSetKeyList.clear();
    this->SetKeyList.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream >> notSetSize;

        this->notSetSize = notSetSize;

        for (int i = 0; i < notSetSize; i++)
        {
            quint16 funIndex = 0; //功能序号：2字节 （0：管理员）
            quint32 funP = 0; //功能权限：4字节
            quint8 status = 0; //状态：1字节   表示：是否停机，注释：0：不停机，1：停机
            readDataStream.readRawData((char*) &funIndex, 2);
            readDataStream.readRawData((char*) &funP, 4);
            readDataStream >> status;

            this->NotSetKeyList.append(
                    MESPDFuncCfg::SetKeyInfo("", funIndex, funP,
                            (MESPDFuncCfg::KeyStatus) status));
        }
        readDataStream >> setSize;
        this->setSize = setSize;

        for (int i = 0; i < setSize; i++)
        {
            char name[9]; //功能名称：8字节
            quint16 funIndex = 0; //功能序号：2字节
            quint32 funP = 0; //功能权限：4字节
            quint8 status = 0;	//状态：1字节 表示：是否停机，注释：0：不停机，1：停机
            memset(name, 0, sizeof(name));
            readDataStream.readRawData((char*) &name, 8);
            readDataStream.readRawData((char*) &funIndex, sizeof(funIndex));
            readDataStream.readRawData((char*) &funP, sizeof(funP));
            readDataStream >> status;

            this->SetKeyList.append(
                    MESPDFuncCfg::SetKeyInfo(name, funIndex, funP,
                            (MESPDFuncCfg::KeyStatus) status));
        }
        fileRead.close();
        return true;
    }
    this->Debug();
    return false;
}


bool MESPDFuncCfg::write(const QString &path)
{
    return false;
}
