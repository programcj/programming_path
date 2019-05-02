/*
 * Producted.h
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#ifndef PRODUCTED_H_
#define PRODUCTED_H_

#include "absentity.h"

namespace entity
{
/**
 * 生产数据 实体类
 * Author: cj 2015-03-03
 */
class Producted: public AbsEntity
{
	int id;	//	id pk auto
	QString DispatchNo;	//		派工单号
	QString DispatchPrior;	//	派工项次
	QString ProcCode;	//			工序代码
	QString StaCode;	//			站别代码
	QString EndCycle;	//			生产数据产生时间
	QList<int> TemperValue; //		N路温度值 List<int>
	int MachineCycle; //		机器周期
	int FillTime; //			填充时间
	int CycleTime; //	CycleTime	成型周期
	int TotalNum; //	TotalNum		模次
	QList<int> KeepPress; //	KeepPress	100个压力点 List<int>
	QString StartCycle; //	StartCycle	生产数据开始时间

DECLARE_COLUMNS_MAP()
	;
public:

	/**
	 * 保存
	 */
	virtual bool subimt();

	/**
	 * 保存到备份表中
	 */
	bool subimt_bak();

	/**
	 * 删除
	 */
	virtual bool remove();

	static bool allList(QList<Producted> &list);
	static int count_bak();
	static int count();
public:
	//SET GET

	QList<int>& getKeepPress()
	{
		return KeepPress;
	}
	QList<int>& getTemperValue()
	{
		return TemperValue;
	}

	void setKeepPress(const QString & keepPressStr)
	{
		//KeepPress = keepPress;
		QStringList sections = keepPressStr.split(',');
		for (int i = 0; i < sections.length(); i++)
		{
			KeepPress.append(sections.at(i).toInt());
		}
	}

	void setTemperValue(const QString& temperValueStr)
	{
		///TemperValue = temperValue;
		QStringList sections = temperValueStr.split(',');
		for (int i = 0; i < sections.length(); i++)
		{
			TemperValue.append(sections.at(i).toInt());
		}
	}

	int getCycleTime() const
	{
		return CycleTime;
	}

	void setCycleTime(int cycleTime)
	{
		CycleTime = cycleTime;
	}

	const QString& getDispatchNo() const
	{
		return DispatchNo;
	}

	void setDispatchNo(const QString& dispatchNo)
	{
		DispatchNo = dispatchNo;
	}

	const QString& getDispatchPrior() const
	{
		return DispatchPrior;
	}

	void setDispatchPrior(const QString& dispatchPrior)
	{
		DispatchPrior = dispatchPrior;
	}

	const QString& getEndCycle() const
	{
		return EndCycle;
	}

	void setEndCycle(const QString& endCycle)
	{
		EndCycle = endCycle;
	}

	int getFillTime() const
	{
		return FillTime;
	}

	void setFillTime(int fillTime)
	{
		FillTime = fillTime;
	}

	int getId() const
	{
		return id;
	}

	void setId(int id)
	{
		this->id = id;
	}

	int getMachineCycle() const
	{
		return MachineCycle;
	}

	void setMachineCycle(int machineCycle)
	{
		MachineCycle = machineCycle;
	}

	const QString& getProcCode() const
	{
		return ProcCode;
	}

	void setProcCode(const QString& procCode)
	{
		ProcCode = procCode;
	}

	const QString& getStaCode() const
	{
		return StaCode;
	}

	void setStaCode(const QString& staCode)
	{
		StaCode = staCode;
	}

	const QString& getStartCycle() const
	{
		return StartCycle;
	}

	void setStartCycle(const QString& startCycle)
	{
		StartCycle = startCycle;
	}

	int getTotalNum() const
	{
		return TotalNum;
	}

	void setTotalNum(int totalNum)
	{
		TotalNum = totalNum;
	}
};

} /* namespace entity */

#endif /* PRODUCTED_H_ */
