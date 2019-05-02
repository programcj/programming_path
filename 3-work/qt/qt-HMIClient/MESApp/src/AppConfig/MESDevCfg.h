/*
 * MESDevCfg.h
 *
 *  Created on: 2015年2月5日
 *      Author: cj
 */

#ifndef MESDEVCFG_H_
#define MESDEVCFG_H_

#include <QtCore>

/*
 *设备配置文件
 */
class MESDevCfg
{
//	版本号：6字节的生成日期(HEX)
//	端口采集顺序：20字节，参考详细设计文档中的“数据采集端口定义”
//	周期采集编号：20字节，两个字节为一组周期的起始和结束编号，共可填入10组采集周期
//  1 2 3 4 5 6

//  1 6 3 5 1 1
//	                       目前增加三个周期：
//	i.	机器周期（2字节）
//	ii.	填充周期（2字节）
//	iii.	成型周期（2字节）
//	iv.	其他保留（14字节）
//	功能采集使能：20字节，8字节对应IO模块每个端口的状态，0为关闭，其他为使能。其他字节保留。
public:
	char version[6];	//版本号：6字节的生成日期(HEX)
	unsigned char portCollect[20];	//端口采集顺序：20字节，参考详细设计文档中的“数据采集端口定义”
	unsigned char CycleCollect[20];	//周期采集编号：20字节，两个字节为一组周期的起始和结束编号，共可填入10组采集周期
	//	目前增加三个周期：
	//i.	机器周期（2字节）The machine cycle
	//ii.	填充周期（2字节） Fill
	//iii.	成型周期（2字节）Forming
	//iv.	其他保留（14字节）Other
	unsigned char FunCollectEnable[20];//功能采集使能：20字节，8字节对应IO模块每个端口的状态，0为关闭，其他为使能。其他字节保留。

	MESDevCfg();
	//机器周期Start
	unsigned char getCycleMachineStart() const
	{
		return CycleCollect[0];
	}
	//机器周期end
	unsigned char getCycleMachineEnd() const
	{
		return CycleCollect[1];
	}
	//填充周期Start
	unsigned char getCycleFillStart() const
	{
		return CycleCollect[2];
	}
	//填充周期end
	unsigned char getCycleFillEnd() const
	{
		return CycleCollect[3];
	}
	//成型周期Start
	unsigned char getCycleFormingStart() const
	{
		return CycleCollect[4];
	}
	//成型周期End
	unsigned char getCycleFormingEnd() const
	{
		return CycleCollect[5];
	}

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESDEVCFG_H_ */
