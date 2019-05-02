/*
 * MESCauseBadCfg.cpp
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#include "MESCauseBadCfg.h"
#include "QtCore"


bool MESCauseBadCfg::read(const QString &path)
{
    //char version[6]; //版本号：6字节的生成日期(HEX)
    quint8 sumSize; //次品类型总数：1字节（n）

    this->map.clear();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);

        readDataStream.readRawData(this->version, sizeof(this->version));
        readDataStream.readRawData((char*) &sumSize, sizeof(sumSize));
        for (int i = 0; i < sumSize; i++)
        {
            quint8 size = 0;	//次品产生原因数量：1字节
            readDataStream >> size;

            QStringList list;
            for (int j = 0; j < size; j++)
            {
                char name[20]; //第1个次品产生原因名字：20字节
                readDataStream.readRawData(name, sizeof(name));
                list.append(QString(name));
            }
            this->map.insert(i, list);
        }
        fileRead.close();
        return true;
    }
    return false;
}


bool MESCauseBadCfg::write(const QString &path)
{
    //版本号：6字节的生成日期(HEX)
    quint8 sumSize=this->map.size(); //次品类型总数：1字节（n）

    QDateTime dateTime = QDateTime::currentDateTime();
    version[0] = dateTime.date().year() % 100;
    version[1] = dateTime.date().month();
    version[2] = dateTime.date().day();
    version[3] = dateTime.time().hour();
    version[4] = dateTime.time().minute();
    version[5] = dateTime.time().second();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::WriteOnly))
    {
        QDataStream dataStream(&fileRead);

        dataStream.writeBytes(this->version, sizeof(this->version));
        dataStream.writeBytes((char*) &sumSize, sizeof(sumSize));
        for (int i = 0; i < sumSize; i++)
        {
            QStringList &list=map[i];
            quint8 size = list.size();	//次品产生原因数量：1字节
            dataStream << size;

            for (int j = 0; j < size; j++)
            {
                char name[20]; //第1个次品产生原因名字：20字节
                strcpy(name, list[j].toAscii().data());
                dataStream.writeBytes(name, sizeof(name));
            }
        }
        fileRead.close();
        return true;
    }
    return false;
}
