/*
 * ADJMachine.h
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#ifndef ADJMACHINE_H_
#define ADJMACHINE_H_

#include "../absentity.h"

namespace entity {

/*
 *ii.	发送调机良品数信息
 */
class ADJMachine: public AbsPropertyBase {
public:
	//调机数据
	class AdjustData: public AbsPropertyBase {
	public:
		QString ItemNO; //	20	ASC	产品编号
		int CurChangeQty; //	4	DWORD	本次调机良品总数
		int NullQty; //	4	DWORD	本次调机空模总数
		int ProdQty; //	4	DWORD	本次调机生产总数
		AdjustData() {
			ItemNO = "";
			CurChangeQty = 0;
			NullQty = 0;
			ProdQty = 0;
		}
		AdjustData(const QString &no, int cur, int null, int prod) {
			ItemNO = no;
			CurChangeQty = cur;
			NullQty = null;
			ProdQty = prod;
		}
		virtual const char *getClassName() const {
			return "AdjustData";
		}
		virtual void onBindProperty() {
			addProperty("ItemNO", Property::AsQStr, &ItemNO);
			addProperty("CurChangeQty", Property::AsInt, &CurChangeQty);
			addProperty("NullQty", Property::AsInt, &NullQty);
			addProperty("ProdQty", Property::AsInt, &ProdQty);
		}
	};
private:
	QString DispatchNo; //	20	ASC	派工单号
	QString DispatchPrior; //	30	ASC	派工项次
	QString ProcCode; //	20	ASC	工序代码
	QString StaCode; //	10	ASC	站别代码
	QString StartCardID; //	10	ASC	开始调机卡号
	QString StartTime; //	6	HEX	开始调机时间
	QString EndCardID; //	10	ASC	结束调机卡号
	QString EndTime; //	6	HEX	结束调机时间
	int CardType; //	1	HEX	调机原因编号[]
	int MultiNum; //	1	HEX	总件数N（0< N <= 100）
	//QString Data; //	N*Adjust_Data_SIZE	HEX	N件产品的调机数据
	QList<AdjustData> adjstDataList;

public:
	virtual const char *getClassName() const {
		return GetThisClassName();
	}

	static const char *GetThisClassName() {
		return "ADJMachine";
	}

	virtual void onBindProperty() {
		addProperty("DispatchNo", Property::AsQStr, &DispatchNo); //	20	ASC	派工单号
		addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior); //	30	ASC	派工项次
		addProperty("ProcCode", Property::AsQStr, &ProcCode); //	20	ASC	工序代码
		addProperty("StaCode", Property::AsQStr, &StaCode); //	10	ASC	站别代码
		addProperty("StartCardID", Property::AsQStr, &StartCardID); //	10	ASC	开始调机卡号
		addProperty("StartTime", Property::AsQStr, &StartTime); //	6	HEX	开始调机时间
		addProperty("EndCardID", Property::AsQStr, &EndCardID); //	10	ASC	结束调机卡号
		addProperty("EndTime", Property::AsQStr, &EndTime); //	6	HEX	结束调机时间
		addProperty("CardType", Property::AsInt, &CardType); //	1	HEX	调机原因编号
		addProperty("MultiNum", Property::AsInt, &MultiNum); //	1	HEX	总件数N（0< N <= 100）
		//addProperty("Data", Property::AsQStr, Data); //	N*Adjust_Data_SIZE	HEX	N件产品的调机数据
		addProperty("adjstDataList", Property::AsQList_AdjustData,
				&adjstDataList);
	}

	QList<AdjustData> &getAdjstDataList() {
		return adjstDataList;
	}

	int getCardType() const {
		return CardType;
	}

	void setCardType(int cardType) {
		CardType = cardType;
	}

	const QString& getDispatchNo() const {
		return DispatchNo;
	}

	void setDispatchNo(const QString& dispatchNo) {
		DispatchNo = dispatchNo;
	}

	const QString& getDispatchPrior() const {
		return DispatchPrior;
	}

	void setDispatchPrior(const QString& dispatchPrior) {
		DispatchPrior = dispatchPrior;
	}

	const QString& getEndCardId() const {
		return EndCardID;
	}

	void setEndCardId(const QString& endCardId) {
		EndCardID = endCardId;
	}

	const QString& getEndTime() const {
		return EndTime;
	}

	void setEndTime(const QString& endTime) {
		EndTime = endTime;
	}

	int getMultiNum() const {
		return MultiNum;
	}

	void setMultiNum(int multiNum) {
		MultiNum = multiNum;
	}

	const QString& getProcCode() const {
		return ProcCode;
	}

	void setProcCode(const QString& procCode) {
		ProcCode = procCode;
	}

	const QString& getStaCode() const {
		return StaCode;
	}

	void setStaCode(const QString& staCode) {
		StaCode = staCode;
	}

	const QString& getStartCardId() const {
		return StartCardID;
	}

	void setStartCardId(const QString& startCardId) {
		StartCardID = startCardId;
	}

	const QString& getStartTime() const {
		return StartTime;
	}

	void setStartTime(const QString& startTime) {
		StartTime = startTime;
	}
};

} /* namespace entity */

#endif /* ADJMACHINE_H_ */
