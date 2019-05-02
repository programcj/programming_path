/*
 * MESCauseBadCfg.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESCAUSEBADCFG_H_
#define MESCAUSEBADCFG_H_

#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>

/*
 *次品产生原因配置文件
 */
class MESCauseBadCfg {
public:
	char version[6];
	QMap<int, QStringList> map;

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESCAUSEBADCFG_H_ */
