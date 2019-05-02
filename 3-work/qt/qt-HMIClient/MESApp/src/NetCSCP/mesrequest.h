#ifndef MESREQUEST_H
#define MESREQUEST_H

#include <QObject>
#include <QtCore>
#include "mespack.h"
#include "../Sqlite/Entity/Producted.h"
#include "../Sqlite/Entity/Order.h"
#include "../Sqlite/Entity/Notebook.h"
#include "../Sqlite/Entity/Material.h"

class MESNet;

extern MESPack::FDATResult FDATResultFormatAppendix(quint8 v);

class MESRequest
{
public:
	int id;
	static QBasicAtomicInt idCounter;

	MESRequest()
	{
		id = idCounter.fetchAndAddRelaxed(1);
	}

	virtual quint8 getCMDSerial()
	{
		return 0x00;
	}

	virtual MESPack toMESPack() = 0;

	virtual bool decode(MESPack &) = 0;
};

class MESResponse
{
public:
	MESPack::FDATResult Result;
	MESResponse()
	{
		Result = MESPack::FDAT_RESULT_NON;
	}
	MESResponse(MESPack::FDATResult Result)
	{
		this->Result = Result;
	}
	virtual MESPack toMESPack() = 0;

	virtual bool decode(MESPack &) = 0;
};
////////////////////////////////////////////////////////////////////////////////////////
struct protocol_reg_info
{
	char FDAT_MACHINE_ID[20];	//	20	ASC	机器ID
	char FDAT_DEV_ID[20];		//	20	ASC	终端ID
	char FDAT_CommPassword[6];	//	6	ASC	终端通讯密码
	char FDAT_SystemType[20];	//	20	ASC	当前系统类型和版本
	char FDAT_SoftwareVer[20];	//	20	ASC	软件版本号
	unsigned long FDAT_FlashSpace;	   //	4	DWORD	Flash可用空间，单位字节
	unsigned char FDAT_SystemTime[6];  //	6	HEX	终端当前时间，若2012-06-20 09:18:32
									   //时间可以为：12,06,20,9,18,32
	unsigned char FDAT_ConfigFileNum;  //	1	HEX	配置文件个数M（0< M <= 10）
	unsigned char FDAT_Reserved[10];   //	10	HEX	保留;
};

struct regdev_echo_info
{
	char FDAT_MACHINE_ID[20]; //	20	ASC	机器ID
	char FDAT_DEV_ID[20]; //	20	ASC	终端ID
	unsigned char FDAT_PMSPermit; //	1	BYTE	给终端的通讯许可：
	//		00：终端状态正常，允许通讯
	//		01：IP地址不存在，拒绝通讯
	//		02：无效
	//		03：配置文件不完整
	char FDAT_LastSystemType[20];	//	20	ASC	最新的系统类型和版本
	char FDAT_LastSofrwareVer[20];	//	20	ASC	最新的软件版本号
	unsigned char FDAT_ServerSystemTime[6];	//	6	HEX	服务器系统时间
	char FDAT_LastConfigFileVer[10][6];	//	HEX	最新的配置文件版本号（根据请求的文件名做应答，不存在的填充0）
};

///////////////////////////////////////////////////////////////////////////////////////////////
//注册请求
class MESRegRequest: public MESRequest
{
public:
	struct protocol_reg_info regInfo;
	MESPack::FDATResult Result;
	MESRegRequest()
	{
		memset(&regInfo, 0, sizeof(regInfo));
	}
	virtual quint8 getCMDSerial()
	{
		return 0x01;
	}
	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack)
	{
		return false;
	}
};

//注册响应
class MESRegResponse: public MESResponse
{
public:
	struct regdev_echo_info echoInfo;
	MESRegResponse()
	{
		memset(&echoInfo, 0, sizeof(regdev_echo_info));
	}

	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//启动文件下载请求
class MESFileDownRequest: public MESRequest
{
public:
	QString FDAT_FileName;	//	30	ASC	需要下载文件名字
	QByteArray FDAT_FileVer;	//	10	ASC	本机现存文件版本号
	quint32 FDAT_FlashSpace;	//	4	DWORD	Flash可用空间，单位字节

	MESFileDownRequest(const QString &name, const QByteArray &ver)
	{
		this->FDAT_FileName = name;
		this->FDAT_FileVer = ver;
	}
	virtual quint8 getCMDSerial()
	{
		return 0x40;
	}
	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack)
	{
		return false;
	}
};

//启动文件下载响应
class MESFileDownResponse: public MESResponse
{
public:
	QString FDAT_FileName;	//	30	ASC	需要下载文件名字
	QByteArray FDAT_LastFileVer;	//	10	ASC	最新的文件版本号
	quint32 FDAT_LastFileSize;	//	4	DWORD	最新的文件的大小
	quint32 FDAT_LastFileChecksum;	//	4	DWORD	文件的CRC32校验值
	quint8 FDAT_Result;	//	1	BYTE	反馈结果
//    0x00：允许下载程序
//    0x01：软件版本号相同，拒绝下载
//    0x02：FLASH空间太小，不足够存放当前应用程序
//    0x03：服务器忙，请稍后再下载
//    0x04：其它未知错误

	MESFileDownResponse()
	{
		FDAT_LastFileSize = 0;
		FDAT_LastFileChecksum = 0;
		FDAT_Result = 0xFF;
	}
	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack);
};
///////////////////////////////////////////////////////////////////////////////////
//文件下载请求
class MESFileDownStreamRequest: public MESRequest
{
public:
	QString FDAT_FileName;	//	30	ASC	需要下载文件名字
	quint32 FDAT_OffsetAddr;	//	4	DWORD	起始偏移地址
	quint16 FDAT_PackSize;	//	2	WORD	本次需要传输的包大小

	MESFileDownStreamRequest(const QString &name, quint32 OffsetAddr,
			quint16 FDAT_PackSize)
	{
		this->FDAT_FileName = name;
		this->FDAT_OffsetAddr = OffsetAddr;
		this->FDAT_PackSize = FDAT_PackSize;
	}
	virtual quint8 getCMDSerial()
	{
		return 0x41;
	}
	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack)
	{
		return false;
	}
};

//    FDAT_CMDSerial	1	BYTE	命令序列号：值固定为(0x41 + 0x80)
//    FDAT_FileName	30	ASC	需要下载文件名字
//    FDAT_OffsetAddr	4	DWORD	起始偏移地址
//    FDAT_Data	N	HEX	数据包
//    FDAT_DataType	1	BYTE	数据类型类型
class MESFileDownStreamResponse: public MESResponse
{
public:
	QString FDAT_FileName;
	quint32 FDAT_OffsetAddr;
	QByteArray data;
	quint8 FDAT_DataType;

	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack);
};

//请求派单
class MESOrderRequest: public MESRequest
{
public:
	entity::Order order;
	quint16 FDAT_DataType;
	QList<entity::Material> MaterialList;
	virtual quint8 getCMDSerial()
	{
		return 0xD0;
	}
	virtual MESPack toMESPack()
	{
		return MESPack(getCMDSerial());
	}
	virtual bool decode(MESPack &pack);
};

class MESOrderResponse: public MESResponse
{
	//FDAT_CMDSerial	1	BYTE	命令序列号：值固定为(0xD0 – 0x80)
	quint16 FDAT_DataType;	//FDAT_DataType	2	HEX	派单类型
	QString FDAT_DispatchNo;	//FDAT_DispatchNo	20	ASC	派工单号
	QString FDAT_DispatchPrior;	//FDAT_DispatchPrior	30	ASC	派工项次
	quint8 FDAT_Result;	//FDAT_Result	1	BYTE	反馈接收结果
//参考附录一
public:
	MESOrderResponse(quint16 FDAT_DataType, const QString &FDAT_DispatchNo,
			const QString &FDAT_DispatchPrior, quint8 FDAT_Result)
	{
		this->FDAT_DataType = FDAT_DataType;
		this->FDAT_DispatchNo = FDAT_DispatchNo;
		this->FDAT_DispatchPrior = FDAT_DispatchPrior;
		this->FDAT_Result = FDAT_Result;
	}

	virtual MESPack toMESPack()
	{
		MESPack pack(0xD0 - 0x80);
		pack.writeQUint16(FDAT_DataType);
		pack.writeQString(FDAT_DispatchNo, 20);
		pack.writeQString(FDAT_DispatchPrior, 30);
		pack.writeQUint8(FDAT_Result);
		return pack;
	}

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
///////////////////////////////////////////////////////////////////////////////////////
//iii.	控制设备
class MESControlRequest: public MESRequest
{
public:
	class OrderIndex
	{
	public:
		int FDAT_SerialNumber;	//	1	HEX	工单序号1
		QString FDAT_DispatchNo;	//	20	ASC	派工单号1
		QString FDAT_DispatchPrior;	//	30	ASC	派工项次1
	};
	class ClassInfo
	{
	public:
		int FDAT_SerialNumber;	//	1	HEX	班次序号1
		quint8 FDAT_BeginTime[2];	//	2	HEX	开始时间1
		quint8 FDAT_EndTime[2];	//	2	HEX	结束时间1

	};
	// FDAT_DataType       控制命令类型
	//        0x00：设备复位
	//        0x01：读取设备RTC时间
	//        0x02：设置设备RTC时间
	//        0x03：读取设备剩余FLASH空间
	//        0x04: 下载文件
	//        0x05: 上传文件
	//        0x06: 设置设备密码
	//        0x07: 调整工单
	//        0x08: 读取程序最新版本号
	//        0x09: 设备换单
	//        0x0A: 设置标准温度
	//        0x0B：设置SQLite语句
	//        0x0C: 配置文件初始化
	//        0x0D: 设置班次
	//		  0x0E: 工单驳回
	//FDAT_CmdCode	20	ASC	控制命令编号,如：201403300001
	quint8 FDAT_DataType; //控制命令类型
	QByteArray FDAT_CmdCode; //控制命令编号

	//0x01：设置RTC时间格式
	QDateTime rtcDateTime;

	//0x04	控制命令类型：下载文件
	QString FDAT_FileName; //	30	ASC	需要下载的文件名
	QByteArray FDAT_FileVer; //20	ASC/HEX	需要下载的文件版本号

	//FDAT_FileName	30	ASC	需要上传的文件名
	//FDAT_Data	10	ASC	设备的操作密码
	QString devPassword;

	//FDAT_AdjustNo	20	ASC	调单流水号
	QByteArray FDAT_AdjustNo;
	QList<OrderIndex> orderIndexList;

	QString sql;
	QString initFileName;
	QList<ClassInfo> classList;

	//工单驳回
	QString FDAT_DispatchNo;	//	20	ASC	派工单号1

	MESControlRequest()
	{
		FDAT_DataType = 0;
		FDAT_FileName = "";
		FDAT_FileVer.clear();
		FDAT_AdjustNo = "";
		devPassword = "";
	}
	virtual quint8 getCMDSerial()
	{
		return 0xD7;
	}
	virtual MESPack toMESPack();
	virtual bool decode(MESPack &pack);

};

class MESControlResponse: public MESResponse
{
public:
	quint8 FDAT_DataType;	//	1	BYTE	控制命令类型
	quint8 FDAT_Result;	//	1	BYTE	反馈接收结果
	QByteArray FDAT_CmdCode;	//	12	ASC	控制命令编号,如：201403300001

	MESControlResponse(quint8 FDAT_DataType, quint8 FDAT_Result,
			QByteArray &FDAT_CmdCode)
	{
		this->FDAT_DataType = FDAT_DataType;
		this->FDAT_Result = FDAT_Result;
		this->FDAT_CmdCode = FDAT_CmdCode;

		rtcTime = QDateTime::currentDateTime();
		flashBack = 0;
	}

	//0x01：读取设备RTC时间
	QDateTime rtcTime;
	//        0x03：读取设备剩余FLASH空间
	quint32 flashBack;
	//        0x07: 调整工单
	//FDAT_AdjustNo	20	ASC	调单流水号
	QByteArray FDAT_AdjustNo;
	//        0x08: 读取程序最新版本号
	//FDAT_SoftwareVer	20	ASC	软件版本号
	QByteArray FDAT_SoftwareVer;
	//0x0B：设置SQLite语句
	//FDAT_ DataLen	2	HEX	返回的数据长度
	//FDAT_ Data	N	ASC	返回的数据内容（字段之间用‘#’号隔开）
	QString sqliteData;
	virtual MESPack toMESPack();

	void bindRequest(MESControlRequest &request);

	virtual bool decode(MESPack &)
	{
		return false;
	}
};

///////////////////////////////////////////////////////////////////////////////////////
class MESNoticeRequest: public MESRequest
{
public:
	QDateTime StartTime;
	QDateTime EndTime;
	QString Text;
	virtual MESPack toMESPack()
	{
		return MESPack();
	}

	virtual bool decode(MESPack &pack);
};
//
///////////////////////////////////////////////////////////////////////////////////////
///ii.	发送调机良品数信息
class MESADJMachineRequest: public MESRequest
{
public:
	entity::ADJMachine adjData;

	MESADJMachineRequest(entity::ADJMachine &v)
	{
		this->adjData = v;
	}

	~MESADJMachineRequest()
	{
		//if (adjData)
		//	delete adjData;
	}
	virtual quint8 getCMDSerial()
	{
		return 0x14;
	}
	virtual MESPack toMESPack();

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
//调机良品数响应
class MESADJMachineResponse: public MESResponse
{
public:
	virtual MESPack toMESPack()
	{
		MESPack pack(0x14 + 0x80);
		return pack;
	}

	virtual bool decode(MESPack &);
};
////////////////////////////////////////////////////////////////////////
//ii.	发送刷卡数据
class MESBrushCardRequest: public MESRequest
{
	QByteArray Body;
public:
	QString DispatchNo; //	20	ASC	派工单号
	QString DispatchPrior; //	30	ASC	派工项次
	QString ProcCode; //	20	ASC	工序代码
	QString StaCode; //	10	ASC	站别代码
	unsigned char CardID[10]; //	10	HEX	卡号
	quint8 CardType; //	1	HEX	刷卡原因编号
	unsigned char CardDate[6]; //	6	HEX	刷卡数据产生时间
	quint8 IsBeginEnd; //	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。

	MESBrushCardRequest(const entity::BrushCard &brush);

	virtual quint8 getCMDSerial()
	{
		return 0x11;
	}
	virtual MESPack toMESPack();

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
////////////////////////////////////////////////////////////////////////
//次品上传
class MESDefectiveInfoRequest: public MESRequest
{
	entity::DefectiveInfo data;
public:
	MESDefectiveInfoRequest(entity::DefectiveInfo &info);
	virtual quint8 getCMDSerial()
	{
		return 0x13;
	}
	virtual MESPack toMESPack();

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
////////////////////////////////////////////////////////////////////////
//发送其它协议
class MESOtherRequest: public MESRequest
{
	MESPack pack;
public:
	MESOtherRequest(entity::OtherSetInfo &info);
	MESOtherRequest();
	void setAsyncOrder(QStringList &orderList);
	virtual quint8 getCMDSerial()
	{
		return 0x15;
	}
	virtual MESPack toMESPack();

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
//响应
class MESOtherResponse: public MESResponse
{
public:
	class OrderIndex
	{
	public:
		int FDAT_SerialNumber; //1	1	HEX	工单序号1
		QString FDAT_DispatchNo; //1	20	ASC	派工单号1
	};

	entity::OtherSetInfo::SetType FDAT_DataType;
	QString FDAT_FileName;
	QList<OrderIndex> orderIndexList;

	virtual MESPack toMESPack()
	{
		MESPack pack;
		return pack;
	}

	virtual bool decode(MESPack &);
};
///////////////////////////////////////////////////////////////////////////
//i.	发送功能记录（巡机数、打磨数等）信息
class MESQualityRegulateRequest: public MESRequest
{
	MESPack pack;
public:
	MESQualityRegulateRequest(entity::QualityRegulate &info);
	virtual quint8 getCMDSerial()
	{
		return 0x16;
	}
	virtual MESPack toMESPack()
	{
		return pack;
	}

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
//////////////////////////////////////////////////////////
//发送生产数据
class MESProduceRequest: public MESRequest
{
	MESPack pack;
public:
	MESProduceRequest(entity::Producted &producted);
	virtual quint8 getCMDSerial()
	{
		return 0x10;
	}
	virtual MESPack toMESPack()
	{
		return pack;
	}

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
////////////////////////////////////////////////////
class MESWIFISignalRequest: public MESRequest
{
	quint8 SignalStrength;
public:
	MESWIFISignalRequest(int signal)
	{
		SignalStrength = signal;
	}
	virtual quint8 getCMDSerial()
	{
		return 0x50;
	}
	virtual MESPack toMESPack()
	{
		MESPack pack(getCMDSerial());
		pack.writeQUint8(SignalStrength);
		return pack;
	}

	virtual bool decode(MESPack &)
	{
		return false;
	}
};
#endif // MESREQUEST_H
