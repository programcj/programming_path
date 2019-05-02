#ifndef QualityRegulate_h_
#define QualityRegulate_h_

#include "../absentity.h"

namespace entity
{

/**
 * 功能记录（巡机数、打磨数等）信息
 */
class QualityRegulate: public AbsPropertyBase
{
public:
	//58、	每件产品的记录数据
	class FuncData: public AbsPropertyBase
	{
	public:
		QString ItemNO;   //	20	ASC	产品编号
		int CurChangeQty; //	4	DWORD	本次记录总数

		virtual const char *getClassName() const
		{
			return "FuncData";
		}

		virtual void onBindProperty()
		{
			addProperty("ItemNO", Property::AsQStr, &ItemNO);
			addProperty("CurChangeQty", Property::AsInt, &CurChangeQty);
		}
	};

private:
	//int ID;//流水号
	QString DispatchNo; //派工单号
	QString DispatchPrior;	//派工顺序号
	QString ProcCode;	//工序代码
	QString StaCode;	//工作中心代码
	QString StartCardNo;	//开始刷卡卡号
	QString StartTime; //开始刷卡时间
	QString EndCardNo; //结束刷卡卡号
	QString EndTime; //结束刷卡时间
	int CardType; //刷卡原因
	//FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
	//QString Data; //N件产品的记录数据
	QList<FuncData> dataList;

public:
	QualityRegulate(void);
	virtual ~QualityRegulate(void);

	virtual const char *getClassName() const
	{
		return GetThisClassName();
	}
	static const char *GetThisClassName()
	{
		return "QualityRegulate";
	}

	virtual void onBindProperty()
	{
		addProperty("DispatchNo", Property::AsQStr, &DispatchNo); //派工单号
		addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);	//派工顺序号
		addProperty("ProcCode", Property::AsQStr, &ProcCode);	//工序代码
		addProperty("StaCode", Property::AsQStr, &StaCode);	//工作中心代码
		addProperty("StartCardNo", Property::AsQStr, &StartCardNo);	//开始刷卡卡号
		addProperty("StartTime", Property::AsQStr, &StartTime); //开始刷卡时间
		addProperty("EndCardNo", Property::AsQStr, &EndCardNo); //结束刷卡卡号
		addProperty("EndTime", Property::AsQStr, &EndTime); //结束刷卡时间
		addProperty("CardType", Property::AsInt, &CardType); //刷卡原因
		addProperty("dataList", Property::AsQList_FuncData, &dataList);
	}

	int getCardType() const
	{
		return CardType;
	}
	//刷卡原因编号:巡机、打磨等 1代表巡机  2代表打磨
	void setCardType(int cardType)
	{
		CardType = cardType;
	}

	QList<FuncData>& getDataList()
	{
		return dataList;
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

	const QString& getEndCardNo() const
	{
		return EndCardNo;
	}

	void setEndCardNo(const QString& endCardNo)
	{
		EndCardNo = endCardNo;
	}

	const QString& getEndTime() const
	{
		return EndTime;
	}

	void setEndTime(const QString& endTime)
	{
		EndTime = endTime;
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

	const QString& getStartCardNo() const
	{
		return StartCardNo;
	}

	void setStartCardNo(const QString& startCardNo)
	{
		StartCardNo = startCardNo;
	}

	const QString& getStartTime() const
	{
		return StartTime;
	}

	void setStartTime(const QString& startTime)
	{
		StartTime = startTime;
	}
};
}

#endif
