/*
 * BrushCard.h
 * 刷卡
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#ifndef BRUSHCARD_H_
#define BRUSHCARD_H_

#include "../absentity.h"
#include "../Order.h"

namespace entity {
/**
 * 刷卡数据实体类
 */
class BrushCard: public AbsPropertyBase {
public:
	enum CardType {
		AsCONFIG_MANAGE = 0, //		0	配置管理	进入配置管理界面，不可配置
		AsQC_NORMAL_CHECK = 1, //		1	QC巡检	默认一定存在，不需要配置（次品录入功能），不可配置
		AsQC_FAULT_REPAIR = 2, //2	故障维修	功能界面不显示，只做维修卡上报
		//3~20	保留
		AsCHANGE_MOLD = 21,			//21  换模
		AsCHANGE_MATERIAL = 22,		//22  换料
		AsCHANGE_ORDER,				//23  换单
		AsAUXILIARY_FAILURE,		//24  辅设故障
		AsMACHINE_FAILURE,			//25  机器故障
		AsMOLD_FAILURE,				//26  模具故障
		AsWAITTING_MATERIAL,		//27  待料
		AsMAINTAIN,					//28  保养
		AsWAITTING_PERSON,			//29  待人
		AsHANDOVER_DUTY,			//30  交接班刷卡
		AsMATERIAL_BAD,				//31  原材料不良
		AsPLANNED_DOWNTIME,			//32  计划停机
		AsON_DUTY,					//33 上班
		AsOFF_DUTY,					//34 下班
		AsCHANGE_STANDARD_CYCLE,	//35  修改周期
		AsDEBUG_MACHINE,			//36  调机
		AsORDER_ADJUST,				//37  调单
		AsSOCKET_NUM,				//38  调整模穴数
		AsENGINEERING_WAIT,  		//39  工程等待
		AsADD_MATERIAL, 			//40  添料
		AsINSPECT_PRO,  			//41  巡机
		AsPOLISH_PRO,  				//42  打磨
		AsORDER_ALLOT,  			//43  工单调拔
		AsMOLD_TESTING,  			//44  试模
		AsDEVICE_CHECK_ITEM,		//45  点检
		AsPO_ELECTRIC_QUANTITY,		//46  用电量
		AsTest_Material,			//47  试料
		AsQUALITY_CHECK = 48,		//48  品质检验
	};

	//31、	修改周期携带数据格式
	class CardStandCycle: public AbsPropertyBase {
	public:
		int StandCycle; //新标准周期，毫秒

		void onBindProperty() {
			addProperty("StandCycle", Property::AsInt, &StandCycle);
		}
		CardStandCycle() {
			StandCycle = 0;
		}
		CardStandCycle(int cycle) {
			this->StandCycle = cycle;
		}
		const char *getClassName() const {
			return "CardStandCycle";
		}
	};
	//32、	工单调拨携带数据格式
	class CardMachineNo: public AbsPropertyBase {
	public:
		QString MachineNo; //新机器编号

		void onBindProperty() {
			addProperty("MachineNo", Property::AsQStr, &MachineNo);
		}
		CardMachineNo() {
			MachineNo = "";
		}
		virtual const char *getClassName() const {
			return "CardMachineNo";
		}
	};
	//33、	机器故障、模具故障、辅设故障刷卡携带数据格式
	class CardFault: public AbsPropertyBase {
	public:
		int FaultNo; //	1	HEX	故障序号
		int ResultNo; //	1	HEX	维修结果序号,或时间   [0 NG]  [1 OK]
		void onBindProperty() {
			addProperty("FaultNo", Property::AsInt, &FaultNo);
			addProperty("ResultNo", Property::AsInt, &ResultNo);
		}

		CardFault() {
			FaultNo = 0;
			ResultNo = 0;
		}

		virtual const char *getClassName() const {
			return "CardFault";
		}
	};
//	//34、	机器故障、模具故障、辅设故障刷结束卡携带数据格式
//	class CardFaultEnd: public AbsPropertyBase
//	{
//	public:
//		QString FaultNo; //20	HEX	故障识别码
//		int ResultNo; //1	HEX	维修结果序号
//
//		void onBindProperty()
//		{
//			addProperty("FaultNo", Property::AsQStr, &FaultNo);
//			addProperty("ResultNo", Property::AsInt, &ResultNo);
//		}
//		CardFaultEnd()
//		{
//			FaultNo = "";
//			ResultNo = 0;
//		}
//
//		virtual const char *getClassName() const
//		{
//			return "CardFaultEnd";
//		}
//	};
	//35、	机器维修数据格式
	class CardFaultNo: public AbsPropertyBase {
	public:
		QString FaultNo; //	20	HEX	故障识别码

		void onBindProperty() {
			addProperty("FaultNo", Property::AsQStr, &FaultNo);
		}
		CardFaultNo() {
			FaultNo = "";
		}
		virtual const char *getClassName() const {
			return "CardFaultNo";
		}
	};
	//36、	上班携带数据格式
	class CardUpWork: public AbsPropertyBase {
	public:
		int WorkType; //	1	HEX	员工的工作类别
		void onBindProperty() {
			addProperty("WorkType", Property::AsInt, &WorkType);
		}
		CardUpWork() {
			WorkType = 0;
		}

		virtual const char *getClassName() const {
			return "CardUpWork";
		}
	};
private:
	//int id; //id PRIMARY KEY autoincrement
	QString DispatchNo; //	20	ASC	派工单号
	QString DispatchPrior; //	30	ASC	派工项次
	QString ProcCode; //	20	ASC	工序代码
	QString StaCode; //	10	ASC	站别代码
	QString CardID; //	10	HEX	卡号
    int cardType; //	1	HEX	刷卡原因编号
	QString CardDate; //	6	HEX	刷卡数据产生时间
	int IsBeginEnd; //	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。
	//int CarryDataLen; //	2	HEX	刷卡携带数据长度（N）
	QString CarryData; //	N	HEX	刷卡携带数据内容

public:
	virtual const char *getClassName() const {
		return GetThisClassName();
	}

	static const char *GetThisClassName() {
		return "BrushCard";
	}

	void onBindProperty() {
		addProperty("DispatchNo", Property::AsQStr, &DispatchNo); //	20	ASC	派工单号
		addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior); //	30	ASC	派工项次
		addProperty("ProcCode", Property::AsQStr, &ProcCode); //	20	ASC	工序代码
		addProperty("StaCode", Property::AsQStr, &StaCode); //	10	ASC	站别代码
		addProperty("CardID", Property::AsQStr, &CardID); //	10	HEX	卡号
        addProperty("CardType", Property::AsInt, &cardType); //	1	HEX	刷卡原因编号
		addProperty("CardDate", Property::AsQStr, &CardDate); //	6	HEX	刷卡数据产生时间
		addProperty("IsBeginEnd", Property::AsInt, &IsBeginEnd); //	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。
		addProperty("CarryData", Property::AsQStr, &CarryData); //	N	HEX	刷卡携带数据内容
	}

	BrushCard();

	BrushCard(const Order &order, const QString &ic, int type,
			const QString &startTime, int flag) {
		setDispatchNo(order.getMoDispatchNo());
		setDispatchPrior(order.getMoDispatchPrior());
		setStaCode(order.getMoStaCode());
		setProcCode(order.getMoProcCode());

		setCardId(ic);
		setCardType((enum CardType)type);
		setCardDate(startTime);
		setIsBeginEnd(flag);
	}

	BrushCard(const Order &order, const QString &ic, enum CardType type,
			const QString &startTime, int flag) {
		setDispatchNo(order.getMoDispatchNo());
		setDispatchPrior(order.getMoDispatchPrior());
		setStaCode(order.getMoStaCode());
		setProcCode(order.getMoProcCode());

		setCardId(ic);
		setCardType(type);
		setCardDate(startTime);
		setIsBeginEnd(flag);
	}

	const QString& getCarryData() const {
		return CarryData;
	}

	void setCarryDataValut(AbsPropertyBase &data) {
		CarryData = PropertyBaseToXML(data).toString();
	}

	bool parseCarryData(AbsPropertyBase &data) {
		return XMLToPropertyBase(CarryData, data);
	}

	const QString& getCardDate() const {
		return CardDate;
	}

	void setCardDate(const QString& cardDate) {
		CardDate = cardDate;
	}

	const QString& getCardId() const {
		return CardID;
	}

	void setCardId(const QString& cardId) {
		CardID = cardId;
	}

	int getCardType() const {
		return cardType;
	}

	void setCardType(enum CardType type) {
        cardType = type;
	}

//	void setCarryData(const QString& carryData) {
//		CarryData = carryData;
//	}

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

	int getIsBeginEnd() const {
		return IsBeginEnd;
	}

	void setIsBeginEnd(int isBeginEnd) {
		IsBeginEnd = isBeginEnd;
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
};

}
/* namespace mes_protocol */

#endif /* BRUSHCARD_H_ */
