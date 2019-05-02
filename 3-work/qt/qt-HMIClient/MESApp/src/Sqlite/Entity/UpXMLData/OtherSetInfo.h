/*
 * OtherSetInfo.h
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#ifndef OTHERSETINFO_H_
#define OTHERSETINFO_H_

#include "../absentity.h"

namespace entity {

/*
 *ii.	发送其它设置信息
 *ii.	0x00：升级文件
 0x01：同步工单
 0x02：调整工单
 0x03：调整模穴数
 0x04：调整原料
 0x05：刷完停机卡计数
 0x06：设备点检
 0x07：电费统计
 *ii.
 */
class OtherSetInfo: public AbsPropertyBase {
public:
	enum SetType { //设置类型
		AsUpgradeFile = 0x00, //：升级文件
		AsAsyncOrder = 0x01, //：同步工单
		AsAdjOrder = 0x02, //：调整工单
		AsAdjSock = 0x03, //：调整模穴数
		AsAdjMaterial = 0x04, //：调整原料
		AsCardStopNo = 0x05, //：刷完停机卡计数
		AsDevInspection = 0x06, //：设备点检
		AsELEFeeCount = 0x07 //：电费统计
	//其他：保留
	};

	//40、	升级文件(Set_Data_SIZE)
	class UpgradeFile: public AbsPropertyBase {
	public:
		QString FileName;	// 30 ASC 升级文件名
		int Result;	// 1 BYTE 反馈结果 参考附录一
		UpgradeFile() {
			FileName = "";
			Result = 0;
		}
		UpgradeFile(const QString &name, int result) {
			FileName = name;
			Result = result;
		}
		virtual const char *getClassName() const {
			return "UpgradeFile";
		}
		virtual void onBindProperty() {
			addProperty("FileName", Property::AsQStr, &FileName);
			addProperty("Result", Property::AsInt, &Result);
		}
	};

	//42、	调整工单(Set_Data_SIZE)
	class AdjOrder: public AbsPropertyBase {
	public:
		int SerialNumber;	//	1	HEX	工单序号1
		QString DispatchNo;	//	20	ASC	派工单号1
		QString DispatchPrior;	//	30	ASC	派工项次1

		AdjOrder() {
			SerialNumber = 0;
			DispatchNo = "";
			DispatchPrior = "";
		}

		AdjOrder(int index, const QString &no, const QString &prior) {
			SerialNumber = index;
			DispatchNo = no;
			DispatchPrior = prior;
		}

		virtual const char *getClassName() const {
			return "AdjOrder";
		}

		virtual void onBindProperty() {
			addProperty("SerialNumber", Property::AsInt, &SerialNumber);
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);
		}
	private:
	};

	//44、	每件产品的数据格式PCS_Data
	class ItemProduct: public AbsPropertyBase {
	public:
		QString MO;	//	20	ASC	工单号
		QString ItemNO;	//	20	ASC	产品编号
		int DispatchQty;	//	4	DWORD	派工数量
		int SocketNum;	//	1	HEX	模穴数
		ItemProduct() :
				MO(""), ItemNO(""), DispatchQty(0), SocketNum(0) {
		}

		ItemProduct(const QString &mo, const QString &itemNo, int disQty,
				int sock) :
				MO(mo), ItemNO(itemNo), DispatchQty(disQty), SocketNum(sock) {
		}

		virtual const char *getClassName() const {
			return "ItemProduct";
		}
		virtual void onBindProperty() {
			addProperty("MO", Property::AsQStr, &MO);
			addProperty("ItemNO", Property::AsQStr, &ItemNO);
			addProperty("DispatchQty", Property::AsInt, &DispatchQty);
			addProperty("SocketNum", Property::AsInt, &SocketNum);
		}
	};

	//43、	调整模穴数(Set_Data_SIZE)
	class AdjSock: public AbsPropertyBase {
	public:
		QString DispatchNo;	// 20 ASC 派工单号
		QString DispatchPrior;	// 30 ASC 派工项次
		QList<ItemProduct> productList;
		AdjSock() {
			DispatchNo = "";
			DispatchPrior = "";
		}
		virtual const char *getClassName() const {
			return "AdjSock";
		}
		virtual void onBindProperty() {
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);
			addProperty("productList", Property::AsQList_ItemProduct,
					&productList);
		}
	};
	//调整原料,每个原料信息
	class ItemMaterial: public AbsPropertyBase {
	public:
		QString MaterialNO;	//	50	ASC	原料1编号
		QString MaterialName;	//	100	ASC	原料1名称
		QString BatchNO;	//	50	ASC	原料1批次号
		int FeedingQty;	//	4	HEX	原料1投料数量
		QString FeedingTime;	//	6	HEX	原料1投料时间
		ItemMaterial() :
				MaterialNO(""), MaterialName(""), BatchNO(""), FeedingQty(0), FeedingTime(
						"") {

		}
		virtual const char *getClassName() const {
			return "ItemMaterial";
		}
		virtual void onBindProperty() {
			addProperty("MaterialNO", Property::AsQStr, &MaterialNO);
			addProperty("MaterialName", Property::AsQStr, &MaterialName);
			addProperty("BatchNO", Property::AsQStr, &BatchNO);
			addProperty("FeedingQty", Property::AsInt, &FeedingQty);
			addProperty("FeedingTime", Property::AsQStr, &FeedingTime);
		}
	};
	//45、	调整原料
	class AdjMaterial: public AbsPropertyBase {
	public:
		//int MaterialCount;//	1	HEX	原料总数（N）
		QString DispatchNo;	//	20	ASC	派工单号
		QString CardID;	//	10	ASC	员工卡号
		QList<ItemMaterial> MaterialList;
		virtual const char *getClassName() const {
			return "AdjMaterial";
		}
		virtual void onBindProperty() {
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("CardID", Property::AsQStr, &CardID);
			addProperty("MaterialList", Property::AsQList_ItemMaterial,
					&MaterialList);
		}
	};
	//46、	刷完停机卡计数
	class CardStopNo: public AbsPropertyBase {
	public:
		QString DispatchNo;	//	20	ASC	派工单号
		QString DispatchPrior;	//	30	ASC	派工项次
		QString ProcCode;	//	20	ASC	工序代码
		QString StaCode;	//	10	ASC	站别代码
		int CardType;		//	1	HEX	刷卡原因编号
		QString CardDate;	//	6	HEX	生产时间

		CardStopNo() :
				DispatchNo(""), DispatchPrior(""), ProcCode(""), StaCode(""), CardType(
						0), CardDate("") {

		}
		virtual const char *getClassName() const {
			return "CardStopNo";
		}
		virtual void onBindProperty() {
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);
			addProperty("ProcCode", Property::AsQStr, &ProcCode);
			addProperty("StaCode", Property::AsQStr, &StaCode);
			addProperty("CardType", Property::AsInt, &CardType);
			addProperty("CardDate", Property::AsQStr, &CardDate);
		}
	};

	//点检项目
	class InspectionProj: public AbsPropertyBase {
	public:
		int NO;		//	1	HEX	点检项目编号
		int Result;	//1	HEX	点检结果1:OK,0:NG
		QString Brand;//	30	ASC	机器品牌
		virtual const char *getClassName() const {
			return "InspectionProj";
		}
		virtual void onBindProperty() {
			addProperty("NO", Property::AsInt, &NO);
			addProperty("Result", Property::AsInt, &Result);
			addProperty("Brand", Property::AsQStr, &Brand);
		}
	};
	//47、	设备点检
	class DevInspection: public AbsPropertyBase {
	public:
		QString DispatchNo;	//	20	ASC	派工单号
		QString DispatchPrior;	//	30	ASC	派工项次
		QString ProcCode;	//	20	ASC	工序代码
		QString StaCode;	//	10	ASC	站别代码
		QString CardID;	//	10	HEX	卡号
		QString CardDate;	//	6	HEX	点检时间
		//int InspectNum;//	1	HEX	点检总数N（0< N <= 100）
		//CC_Data	N*PCS_Data_SIZE	HEX	N个点检项目
		QList<InspectionProj> nDataList;
		virtual const char *getClassName() const {
			return "DevInspection";
		}
		virtual void onBindProperty() {
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);
			addProperty("ProcCode", Property::AsQStr, &ProcCode);
			addProperty("StaCode", Property::AsQStr, &StaCode);
			addProperty("CardID", Property::AsQStr, &CardID);
			addProperty("CardDate", Property::AsQStr, &CardDate);
			addProperty("nDataList", Property::AsQList_InspectionProj,
					&nDataList);
		}
	};

	//49、	电费统计
	class ELEFeeCount: public AbsPropertyBase {
	public:
		QString DispatchNo;		//	20	ASC	派工单号
		QString DispatchPrior;		//	30	ASC	派工项次
		QString ProcCode;		//	20	ASC	工序代码
		QString StaCode;		//	10	ASC	站别代码
		QString CardID;		//	10	HEX	录入人卡号
		QString CardDate;		//	6	HEX	录入时间
		int ElecNum;		//	4	HEX	当前班次电费
		ELEFeeCount() :
				ElecNum(0) {

		}
		virtual const char *getClassName() const {
			return "ELEFeeCount";
		}
		virtual void onBindProperty() {
			addProperty("DispatchNo", Property::AsQStr, &DispatchNo);
			addProperty("DispatchPrior", Property::AsQStr, &DispatchPrior);
			addProperty("ProcCode", Property::AsQStr, &ProcCode);
			addProperty("StaCode", Property::AsQStr, &StaCode);
			addProperty("CardID", Property::AsQStr, &CardID);
			addProperty("CardDate", Property::AsQStr, &CardDate);
			addProperty("ElecNum", Property::AsInt, &ElecNum);
		}
	};
private:
	//int id;
//	0x00：升级文件
//	 0x01：同步工单(UI调用 Thread)
//	 0x02：调整工单
//	 0x03：调整模穴数
//	 0x04：调整原料
//	 0x05：刷完停机卡计数
//	 0x06：设备点检
//	 0x07：电费统计
	int DataType;
	QString FDAT_Data;

	void setDataType(int dataType) {
		DataType = dataType;
	}

	void setFdatData(const QString& fdatData) {
		FDAT_Data = fdatData;
	}
public:
	virtual const char *getClassName() const {
		return GetThisClassName();
	}
	static const char *GetThisClassName() {
		return "OtherSetInfo";
	}
	virtual void onBindProperty() {
		addProperty("DataType", Property::AsInt, &DataType);
		addProperty("FDAT_Data", Property::AsQStr, &FDAT_Data);
	}

	const QString& getFdatData() const {
		return FDAT_Data;
	}

	int getDataType() const {
		return DataType;
	}

	//40、	升级文件
	QString setUpgradeFile(UpgradeFile &file) {
		setDataType(AsUpgradeFile);
		setFdatData(PropertyBaseToXML(file).toString());
		return getFdatData();
	}

	bool toUpgradeFile(UpgradeFile &file) {
		return XMLToPropertyBase(getFdatData(), file);
	}

	//42、	调整工单
	QString setAdjOrderList(QList<AdjOrder> &list) {
		QDomDocument doc;
		setDataType(AsAdjOrder);
		setFdatData(PropertyListToXML(doc, list).toString());
		return getFdatData();
	}

	bool toAdjOrderList(QList<AdjOrder> &list) {
		XMLToPropertList(getFdatData(), list);
		return true;
	}

	//43、	调整模穴数
	QString setAdjSock(AdjSock &info) {
		setDataType(AsAdjSock);
		setFdatData(PropertyBaseToXML(info).toString());
		return getFdatData();
	}

	bool toAdjSock(AdjSock &info) {
		return XMLToPropertyBase(getFdatData(), info);
	}

	//45、	调整原料
	QString setAdjMaterial(AdjMaterial &info) {
		setDataType(AsAdjMaterial);
		setFdatData(PropertyBaseToXML(info).toString());
		return getFdatData();
	}

	bool toAdjMaterial(AdjMaterial &info) {
		return XMLToPropertyBase(getFdatData(), info);
	}

	//46、	刷完停机卡计数
	QString setCardStopNo(CardStopNo &info) {
		setDataType(AsCardStopNo);
		setFdatData(PropertyBaseToXML(info).toString());
		return getFdatData();
	}

	bool toCardStopNo(CardStopNo &info) {
		return XMLToPropertyBase(getFdatData(), info);
	}

	//47、	设备点检
	QString setDevInspection(DevInspection &info) {
		setDataType(AsDevInspection);
		setFdatData(PropertyBaseToXML(info).toString());
		return getFdatData();
	}

	bool toDevInspection(DevInspection &info) {
		return XMLToPropertyBase(getFdatData(), info);
	}

	//49、	电费统计
	QString setELEFeeCount(ELEFeeCount &info) {
		setDataType(AsELEFeeCount);
		setFdatData(PropertyBaseToXML(info).toString());
		return getFdatData();
	}

	bool toELEFeeCount(ELEFeeCount &info) {
		return XMLToPropertyBase(getFdatData(), info);
	}
};

} /* namespace entity */

#endif /* OTHERSETINFO_H_ */
