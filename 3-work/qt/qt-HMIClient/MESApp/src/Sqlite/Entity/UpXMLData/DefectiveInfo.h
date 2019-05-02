/*
 * DefectiveInfo.h
 *
 *  Created on: 2015年1月28日
 *      Author: cj
 */

#ifndef DEFECTIVEINFO_H_
#define DEFECTIVEINFO_H_

#include "../absentity.h"

namespace entity
{

/*
 * 次品信息 实体类
 */
class DefectiveInfo: public AbsPropertyBase
{
public:
	//每件产品的数据格式
	class ProductInfo: public AbsPropertyBase
	{
		QString ItemNo; //	20	ASC	产品编号
		//int BadQty; //	4	DWORD	次品总数
		QList<int> BadData; //	M *4	HEX	每件产品的次品数据（对应次品原因的数据）
	public:
		virtual const char *getClassName() const
		{
			return "ProductInfo";
		}
		void onBindProperty()
		{
			addProperty("ItemNo", Property::AsQStr, &ItemNo);
			addProperty("BadData", Property::AsQListInt, &BadData);
		}
		ProductInfo()
		{
			ItemNo = "";
		}

		const QString& getItemNo() const
		{
			return ItemNo;
		}

		void setItemNo(const QString& itemNo)
		{
			ItemNo = itemNo;
		}

		QList<int>& getBadData()
		{
			return BadData;
		}

		int getBadQty() const
		{
			int sum = 0;
			for (int i = 0; i < BadData.size(); i++)
				sum += BadData[i];
			return sum;
		}
	};

private:
	//int id; //integer PRIMARY KEY autoincrement
	QString DispatchNo; //	20	ASC	派工单号
	QString DispatchPrior; //	30	ASC	派工项次
	QString ProcCode; //	20	ASC	工序代码
	QString StaCode; //	10	ASC	站别代码
	QString CardID; //	10	HEX	卡号
	QString CardDate; //	6	HEX	次品数据产生时间
	int Status; //	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
	int MultiNum; //	1	HEX	总件数N（0< N <= 100）
	int BadReasonNum; //	1	HEX	次品原因总数M（0< M <= 100）
	//QString Data; //	N*Bad_Data_SIZE	HEX	N件产品的次品数据
	int FDAT_BillType;	//	1	HEX	单据类别(0表示次品,1表示隔离品)
	QList<ProductInfo> badDataList;

public:
	QList<ProductInfo> &getBadDataList()
	{
		return badDataList;
	}

	virtual const char *getClassName() const
	{
		return GetThisClassName();
	}
	static const char *GetThisClassName()
	{
		return "DefectiveInfo";
	}
	void onBindProperty()
	{
		addProperty("DispatchNo", Property::AsQStr, &DispatchNo); //	20	ASC	派工单号
		addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior); //	30	ASC	派工项次
		addProperty("ProcCode", Property::AsQStr, &ProcCode); //	20	ASC	工序代码
		addProperty("StaCode", Property::AsQStr, &StaCode); //	10	ASC	站别代码
		addProperty("CardID", Property::AsQStr, &CardID); //	10	HEX	卡号
		addProperty("CardDate", Property::AsQStr, &CardDate); //	6	HEX	次品数据产生时间
		addProperty("Status", Property::AsInt, &Status); //	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
		addProperty("MultiNum", Property::AsInt, &MultiNum); //	1	HEX	总件数N（0< N <= 100）
		addProperty("BadReasonNum", Property::AsInt, &BadReasonNum); //	1	HEX	次品原因总数M（0< M <= 100）
		addProperty("FDAT_BillType", Property::AsInt, &FDAT_BillType); //单据类别(0表示次品,1表示隔离品)
		addProperty("badDataList", Property::AsQList_ProductInfo, &badDataList);
	}

	void setBadDataList(QList<ProductInfo> badDataList)
	{
		this->badDataList = badDataList;
	}

	int getBadReasonNum() const
	{
		return BadReasonNum;
	}

	void setBadReasonNum(int badReasonNum)
	{
		BadReasonNum = badReasonNum;
	}

	const QString& getCardDate() const
	{
		return CardDate;
	}

	void setCardDate(const QString& cardDate)
	{
		CardDate = cardDate;
	}

	const QString& getCardId() const
	{
		return CardID;
	}

	void setCardId(const QString& cardId)
	{
		CardID = cardId;
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

	int getMultiNum() const
	{
		return MultiNum;
	}

	void setMultiNum(int multiNum)
	{
		MultiNum = multiNum;
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

	int getStatus() const
	{
		return Status;
	}

	void setStatus(int status)
	{
		Status = status;
	}
	//单据类别(0表示次品,1表示隔离品)
	int getFdatBillType() const
	{
		return FDAT_BillType;
	}
	//单据类别(0表示次品,1表示隔离品)
	void setFdatBillType(int fdatBillType)
	{
		FDAT_BillType = fdatBillType;
	}
};

} /* namespace mes_protocol */

#endif /* DEFECTIVEINFO_H_ */
