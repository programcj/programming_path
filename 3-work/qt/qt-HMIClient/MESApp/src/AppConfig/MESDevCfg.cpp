/*
 * MESDevCfg.cpp
 *
 *  Created on: 2015年2月5日
 *      Author: cj
 */

#include "MESDevCfg.h"
#include "string.h"

MESDevCfg::MESDevCfg()
{
	memset(version, 0, sizeof(version));
	memset(portCollect, 0, sizeof(portCollect));
	memset(CycleCollect, 0, sizeof(CycleCollect));
    memset(FunCollectEnable, 0, sizeof(FunCollectEnable));
}

bool MESDevCfg::write(const QString &path)
{
    //char version[6];	//版本号：6字节的生成日期(HEX)
    //char portCollect[20];	//端口采集顺序：20字节，参考详细设计文档中的“数据采集端口定义”
    //char CycleCollect[20];	//周期采集编号：20字节，两个字节为一组周期的起始和结束编号，共可填入10组采集周期
//	目前增加三个周期：
//i.	机器周期（2字节）
//ii.	填充周期（2字节）
//iii.	成型周期（2字节）
//iv.	其他保留（14字节）
//char FunCollectEnable[20];//功能采集使能：20字节，8字节对应IO模块每个端口的状态，0为关闭，其他为使能。其他字节保留。

    QDateTime dateTime = QDateTime::currentDateTime();
    version[0] = dateTime.date().year() % 100;
    version[1] = dateTime.date().month();
    version[2] = dateTime.date().day();
    version[3] = dateTime.time().hour();
    version[4] = dateTime.time().minute();

    QFile fileRead(path);
    if (fileRead.open(QIODevice::WriteOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.writeRawData((char*) this->version, sizeof(this->version));
        readDataStream.writeRawData((char*) this->portCollect, sizeof(this->portCollect));
        readDataStream.writeRawData((char*) this->CycleCollect, sizeof(this->CycleCollect));
        readDataStream.writeRawData((char*) this->FunCollectEnable, sizeof(this->FunCollectEnable));
        fileRead.close();
        return true;
    }
    return false;
}

bool MESDevCfg::read(const QString &path)
{
    //char version[6];	//版本号：6字节的生成日期(HEX)
    //char portCollect[20];	//端口采集顺序：20字节，参考详细设计文档中的“数据采集端口定义”
    //char CycleCollect[20];	//周期采集编号：20字节，两个字节为一组周期的起始和结束编号，共可填入10组采集周期
//	目前增加三个周期：
//i.	机器周期（2字节）
//ii.	填充周期（2字节）
//iii.	成型周期（2字节）
//iv.	其他保留（14字节）
    //char FunCollectEnable[20];//功能采集使能：20字节，8字节对应IO模块每个端口的状态，0为关闭，其他为使能。其他字节保留。
//QString str=QCoreApplication::applicationDirPath()+ "/config_file/dev_cfg.bin";
    QFile fileRead(path);
    if (fileRead.open(QIODevice::ReadOnly))
    {
        QDataStream readDataStream(&fileRead);
        readDataStream.readRawData((char*) this->version, sizeof(this->version));
        readDataStream.readRawData((char*) this->portCollect,
                sizeof(this->portCollect));
        readDataStream.readRawData((char*) this->CycleCollect,
                sizeof(this->CycleCollect));
        readDataStream.readRawData((char*) this->FunCollectEnable,
                sizeof(this->FunCollectEnable));
        fileRead.close();
        return true;
    }
    return false;
}
