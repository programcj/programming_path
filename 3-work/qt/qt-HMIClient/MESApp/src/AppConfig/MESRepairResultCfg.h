/*
 * MESRepairResultCfg.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESREPAIRRESULTCFG_H_
#define MESREPAIRRESULTCFG_H_

#include "MESFaultTypeCfg.h"
/*
 *故障类型配置文件
 */
class MESRepairResultCfg: public MESFaultTypeCfg {
public:
    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESREPAIRRESULTCFG_H_ */
