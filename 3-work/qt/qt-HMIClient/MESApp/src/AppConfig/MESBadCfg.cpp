/*
 * MESDefectiveReason.cpp
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#include "MESBadCfg.h"

bool MESBadCfg::write(const QString &path)
{
	QFile fileRead(path);

	if (fileRead.open(QIODevice::WriteOnly))
	{
		QDataStream dataStream(&fileRead);
		//char version[6];	//版本号：6字节的生成日期(HEX)
        QDateTime dateTime = QDateTime::currentDateTime();
        version[0] = dateTime.date().year() % 100;
        version[1] = dateTime.date().month();
        version[2] = dateTime.date().day();
        version[3] = dateTime.time().hour();
        version[4] = dateTime.time().minute();
        version[5] = dateTime.time().second();

		quint8 sumSize = name.size(); //次品类型总数：1字节（n）
		quint8 defaultType = this->defaultType; //默认类型：1字节(<n)
		dataStream.writeBytes(this->version, sizeof(this->version));
		dataStream << sumSize << defaultType;

		this->sumSize = sumSize;

		for (int i = 0; i < sumSize; i++)
		{
			quint8 sum = i;	//第1种类型次品：//次品数量：1字节（M1 < = 30）
			dataStream << sum;
			QStringList &list = name[i];

			for (int j = 0; j < sum; j++)
			{
				char name[13];	//第1个次品名字：12字节
				strcpy(name, list[j].toAscii().data());
				dataStream.writeRawData(name, 12);
				list.append(QString(name));
			}
		}
		fileRead.close();
		return true;
	}
	return false;
}

bool MESBadCfg::read(const QString &path)
{
	QFile fileRead(path);

	name.clear();

	if (fileRead.open(QIODevice::ReadOnly))
	{
		QDataStream readDataStream(&fileRead);
		//char version[6];	//版本号：6字节的生成日期(HEX)
		quint8 sumSize = 0; //次品类型总数：1字节（n）
		quint8 defaultType = 0; //默认类型：1字节(<n)
		readDataStream.readRawData(this->version, sizeof(this->version));
		readDataStream >> sumSize >> defaultType;

		sumSize = sumSize;
		defaultType = defaultType;

		for (int i = 0; i < sumSize; i++)
		{
			quint8 sum;	//第1种类型次品：//次品数量：1字节（M1 < = 30）
			readDataStream >> sum;
			QStringList list;
			for (int j = 0; j < sum; j++)
			{
				char name[13];	//第1个次品名字：12字节
				memset(name, 0, 13);
				readDataStream.readRawData(name, 12);
				list.append(QString(name));
			}
			name.insert(i, list);
		}
		fileRead.close();
		return true;
	}
	return false;
}
