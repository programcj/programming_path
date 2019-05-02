/*
 * MESClass.h
 *
 *  Created on: 2015年4月15日
 *      Author: cj
 */

#ifndef MESCLASSCFG_H_
#define MESCLASSCFG_H_

#include <QtCore>

/*
 * 班次配置信息
 */
class ClassTime {
public:
	int wHour;
	int wMinute;
};

class MESClassCfg {
public:
	MESClassCfg();
	QList<ClassTime> ClassTimeList; //班次时间表
	QString ChangeTime; //当前换班时间

	//初始化
	void init();

	//保存功能
	void save();

//    virtual bool write(const QString &path); //配置写入到文件
//    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESCLASS_H_ */
