/*
 * Tool.h
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#ifndef TOOL_H_
#define TOOL_H_

#include <QtCore>
#include <QDateTime>
#include <string.h>
#include "../HAL/buzzer/buzzer.h"

namespace unit
{

/*
 * 工具类集合
 */
class Tool
{
public:

	/**
	 * 取wifi信号
	 */
	static int WirelessSignal();
	static bool getAppState();

	static QString DateTimeToQString(const QDateTime &dateTime)
	{
		return dateTime.toString("yyyy-MM-dd hh:mm:ss");
	}

	static QDateTime QStringToDateTime(const QString &dateTimeStr)
	{
		return QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
	}

	static QString GetCurrentDateTimeStr()
	{
		return DateTimeToQString(QDateTime::currentDateTime());
	}

	/**
	 * 把IC的HEX字符串格式转换成char[6]，可用于协议通信
	 */
	static void ICCardIDFromQString(unsigned char v[6], const QString &icID)
	{
		bool ok;
		//qDebug() << icID;
		quint32 id = icID.toInt(&ok, 16);
		if (ok == false)
		{
			sscanf(icID.toAscii().data(), "%x", &id);
		}
		for (int i = 0; i < 4; i++)
		{
			v[i] = ((char*) &id)[3 - i];
		}
	}

	/**
	 * 把DateTime的字符串格式转换成 char[6]中,分别为年月日时分秒，可用于协议通信
	 */
	static void DateTimeformatCharArray(unsigned char v[6],
			const QString &dateTimeStr)
	{
		QDateTime dateTime = QStringToDateTime(dateTimeStr);
		v[0] = dateTime.date().year() % 100;
		v[1] = dateTime.date().month();
		v[2] = dateTime.date().day();
		v[3] = dateTime.time().hour();
		v[4] = dateTime.time().minute();
		v[5] = dateTime.time().second();
	}

	//整形转QString
	static QString IntToQString(int v)
	{
		QString str;
		str.sprintf("%d", v);
		return str;
	}

	//执行蜂鸣器
	static void ExeBuzzer(){
		Buzzer bz;
		bz.startBuzzer();
	}
};

} /* namespace unit */

#endif /* TOOL_H_ */
