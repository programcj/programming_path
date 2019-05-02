/*
 * MESDefectiveReason.h
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#ifndef MESDEFECTIVEREASON_H_
#define MESDEFECTIVEREASON_H_

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>

#include "../Unit/MESLog.h"

/*
 * 次品原因配置
 */
class MESBadCfg {
public:
	char version[6]; //版本号：6字节的生成日期(HEX)
	int sumSize;  //次品类型总数：1字节（n）
	int defaultType; //默认类型：1字节(<n)
	QMap<int, QStringList> name;

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESDEFECTIVEREASON_H_ */
