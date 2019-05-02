/*
 * MESCheckItemCfg.cpp
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#include "MESCheckItemCfg.h"
#include <QtCore>


bool MESCheckItemCfg::write(const QString &path)
{
    quint8 checkSize=this->List.size(); //项目总数：1字节（M）
    QDateTime dateTime = QDateTime::currentDateTime();
    version[0] = dateTime.date().year() % 100;
    version[1] = dateTime.date().month();
    version[2] = dateTime.date().day();
    version[3] = dateTime.time().hour();
    version[4] = dateTime.time().minute();
    version[5] = dateTime.time().second();
    //this->List.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::WriteOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.writeBytes(this->version, sizeof(this->version));
        readDataStream.writeBytes((char*) &checkSize, sizeof(checkSize));

        this->sumSize = checkSize;

        for (int i = 0; i < checkSize; i++)
        {
            char modeName[30]; //型号名称：XXXX：30字节
            quint8 modeType; //项目类型：0代表“机器”：1字节
            quint8 count; //点检项目数量：1字节（M11）

            MESCheckItemCfg::CheckModel &model=List[i];

            strcpy(modeName,model.ModelName.toAscii().data());
            modeType=0;
            count=model.MachineList.size();

            readDataStream.writeBytes(modeName, 30);
            readDataStream.writeBytes((char*) &modeType, 1); //项目类型：0代表“机器”：1字节
            readDataStream.writeBytes((char*) &count, 1); //点检项目数量：1字节（M11）

            for (int j = 0; j < count; j++)
            {
                MESCheckItemCfg::CheckItem &item=model.MachineList[j];
                quint8 Index; //第1个项目序号：1字节
                char name[30]; //第1个项目：30字节

                Index=item.getIndex();
                strcpy(name, item.getName().toAscii().data());

                readDataStream.writeBytes((char*) &Index, sizeof(Index));
                readDataStream.writeBytes(name, sizeof(name));
            }

            modeType=1;
            count=model.ModeList.size();

            readDataStream.writeBytes((char*) &modeType, 1); //项目类型：0代表“机器”：1字节
            readDataStream.writeBytes((char*) &count, 1); //点检项目数量：1字节（M11）

            for (int j = 0; j < count; j++)
            {
                MESCheckItemCfg::CheckItem &item=model.ModeList[j];
                quint8 Index; //第1个项目序号：1字节
                char name[30]; //第1个项目：30字节

                Index=item.getIndex();
                strcpy(name, item.getName().toAscii().data());

                readDataStream.writeBytes((char*) &Index, sizeof(Index));
                readDataStream.writeBytes(name, sizeof(name));
            }

            //this->List.append(model);
        }
        fileRead.close();
        return true;
    }
    return false;
}

bool MESCheckItemCfg::read(const QString &path)
{
    //char version[6]; //版本号：6字节的生成日期(HEX)
    quint8 checkSize; //项目总数：1字节（M）

    this->List.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream.readRawData((char*) &checkSize, sizeof(checkSize));

        this->sumSize = checkSize;

        for (int i = 0; i < checkSize; i++)
        {
            char modeName[30]; //型号名称：XXXX：30字节
            quint8 modeType; //项目类型：0代表“机器”：1字节
            quint8 count; //点检项目数量：1字节（M11）
            readDataStream.readRawData(modeName, 30);
            readDataStream.readRawData((char*) &modeType, 1); //项目类型：0代表“机器”：1字节
            readDataStream.readRawData((char*) &count, 1); //点检项目数量：1字节（M11）

            MESCheckItemCfg::CheckModel model;
            model.ModelName = modeName;

            for (int j = 0; j < count; j++)
            {
                quint8 Index; //第1个项目序号：1字节
                char name[30]; //第1个项目：30字节

                readDataStream.readRawData((char*) &Index, sizeof(Index));
                readDataStream.readRawData(name, sizeof(name));

                MESCheckItemCfg::CheckItem item(Index, name);
                if (modeType == 0)
                    model.MachineList.append(item);
                if (modeType == 1)
                    model.ModeList.append(item);
            }

            readDataStream.readRawData((char*) &modeType, 1); //项目类型：1代表“模具”：1字节
            readDataStream.readRawData((char*) &count, 1); //点检项目数量：1字节（M12）

            for (int j = 0; j < count; j++)
            {
                quint8 Index; //第1个项目序号：1字节
                char name[30]; //第1个项目：30字节

                readDataStream.readRawData((char*) &Index, sizeof(Index));
                readDataStream.readRawData(name, sizeof(name));

                MESCheckItemCfg::CheckItem item(Index, name);
                if (modeType == 0)
                    model.MachineList.append(item);
                if (modeType == 1)
                    model.ModeList.append(item);
            }

            this->List.append(model);
        }
        fileRead.close();
        return true;
    }
    return false;
}
