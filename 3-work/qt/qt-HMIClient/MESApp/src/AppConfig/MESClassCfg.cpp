/*
 * MESClass.cpp
 *
 *  Created on: 2015年4月15日
 *      Author: cj
 */

#include "MESClassCfg.h"
#include "AppInfo.h"

MESClassCfg::MESClassCfg() {
	ChangeTime = "";
}

void MESClassCfg::init() {
	ClassTimeList.clear();
	QString filePath = AppInfo::getPath_Config() + "/class_data_info.txt";
	QFile fileRead(filePath);
	if (fileRead.open(QIODevice::ReadOnly)) {
		QTextStream stream(&fileRead);
		QString line = fileRead.readLine();
		if (line.length() > 0) {
			ChangeTime = line;

			line = fileRead.readLine();
			QStringList list = line.split("|");
			for (int i = 0; i < list.size(); i++) {
				if (list[i].length() < 2)
					break;

				QStringList timList = list[i].split(":");
				if(timList.size()<2)
				{
					logErr(QString("换班时间文件读取出错:%1").arg(list[i]));
					break;
				}
				ClassTime time;
				time.wHour = timList[0].toInt();
				time.wMinute = timList[1].toInt();
				ClassTimeList.append(time);
			}
		}
		fileRead.close();
	}
}

void MESClassCfg::save() {
	QString filePath = AppInfo::getPath_Config() + "/class_data_info.txt";

	QFile fileRead(filePath);
	if (fileRead.open(QIODevice::WriteOnly)) {
		QTextStream stream(&fileRead);
		stream << ChangeTime << "\r\n";
		for (int i = 0; i < ClassTimeList.size(); i++) {
			stream << ClassTimeList[i].wHour << ":" << ClassTimeList[i].wMinute
					<< "|";
		}
		fileRead.close();
    }
}
