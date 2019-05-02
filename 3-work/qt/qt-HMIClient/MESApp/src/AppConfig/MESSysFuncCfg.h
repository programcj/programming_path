/*
 * MESSysFunc.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESSYSFUNC_H_
#define MESSYSFUNC_H_

#include <QtCore>
#include <QString>

/*
 * 系统功能配置文件
 *版本号：6字节的生成日期(HEX)
 功能总数：1字节（n）
 第1功能：
 功能名称：30字节
 功能序号：2字节
 功能值：2字节
 第2功能：
 功能名称：30字节
 功能序号：2字节
 功能值：2字节

 ……

 第n功能：
 功能名称：30字节
 功能序号：2字节
 功能值：2字节
 sys_func_cfg
 *
 */
class MESSysFuncCfg
{
	class FunInfo
	{
	public:
		QString name; //功能名称：30字节
		int index; //功能序号：2字节
		int value; //功能值：2字节
	};
//	序号	功能名称	功能描述
//	1	停机是否计数	在刷完停机卡后是否需要计数，0：不计数，1：计数
//	2	采集压力方式	1：电压，2：电流。
//	3	是否允许多种停机状态	0：不允许，1：允许
//	4	达到生产数是否自动换单	0：不换单，1：自动换单
//	5	信号采集方式	0：采集器DI采集 ，1：IO模块DI采集
//	6	机台信号类型	0：通用类型，1：德玛格机型，2：柳州开宇，3：日钢，4：名机（大机）
//	7	压力是否接线	0：未接线，1：已接线
//	8	温度路数	2：2路温度，4：4路温度，8：8路温度
public:
	char version[6]; //	版本号：6字节的生成日期(HEX)
	int funSize; //	功能总数：1字节（n）

	void SetFun(const char funName[30], //功能名称：30字节
			quint16 funIndex, //功能序号：2字节
			quint16 funValue); //功能值：2字节
	//	1	停机是否计数	在刷完停机卡后是否需要计数，0：不计数，1：计数
	bool isStopCount()
	{
		return funArrays[1].value == 1;
	}
	//	2	采集压力方式	1：电压，2：电流。
	bool PressType()
	{
	       return funArrays[2].value;
	}

	//	3	是否允许多种停机状态	0：不允许，1：允许
	//			多停机: A 停机, B 也可以停机
	//			单停机: A 停机     B 不可以停机
	bool isMultipleStopStatus()
	{
		return funArrays[3].value == 1;
	}
	//	4	达到生产数是否自动换单	0：不换单，1：自动换单
	bool isAutoChangeOrder()
	{
		return funArrays[4].value == 1;
	}
	//	5	信号采集方式	0：采集器DI采集 ，1：IO模块DI采集
	bool isIOCollect()
	{
		return funArrays[5].value == 1;
	}

	//	6	机台信号类型	0：通用类型，1：德玛格机型，2：柳州开宇，3：日钢，4：名机（大机）
	quint8 machineType()
	{
		return funArrays[6].value ;
	}
	//	7	压力是否接线	0：未接线，1：已接线
	bool isLineforPress()
	{
		return funArrays[7].value ;
	}
	//	8	温度路数	2：2路温度，4：4路温度，8：8路温度
	quint8 temperateChannel()
	{
	      return funArrays[8].value ;
	}

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
private:
	FunInfo funArrays[9];
};

#endif /* MESSYSFUNC_H_ */
