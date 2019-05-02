#include "mesnet.h"
#include "mesrequest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

QBasicAtomicInt MESRequest::idCounter = Q_BASIC_ATOMIC_INITIALIZER(1);

//附录一 转换成 MESPack::FDATResult
MESPack::FDATResult FDATResultFormatAppendix(quint8 v)
{
	MESPack::FDATResult result = MESPack::FDAT_RESULT_NON;
	switch (v)
	{
	case 0x00:
		result = MESPack::FDAT_RESULT_SUCESS;
		break; //	接收成功，并保存
	case 0x01:
		result = MESPack::FDAT_RESULT_FAILED;
		break; //	接收数据有问题，不需要重发
	case 0x02:
		result = MESPack::FDAT_RESULT_RESTART;
		break; //	接收数据有问题，需要重发
	case 0x03:
		result = MESPack::FDAT_RESULT_NOT_PERMISSION;
		break; //	无权限
	case 0x04:
		result = MESPack::FDAT_RESULT_SUCESS_SWING_CARD;
		break; //	刷卡成功
	case 0x05:
		result = MESPack::FDAT_RESULT_NOT_DOWN_WORK;
		break; //	没刷下班卡
	case 0x06:
		result = MESPack::FDAT_RESULT_NOT_UP_WORK;
		break; //	没刷上班卡
	case 0x07:
		result = MESPack::FDAT_RESULT_NON_SWING_CARD;
		break; //	刷卡未知错误
	case 0x08:
		result = MESPack::FDAT_RESULT_SUCESS_UPFILE;
		break; //	升级文件成功
	case 0x09:
		result = MESPack::FDAT_RESULT_FAILED_UPFILE;
		break; //	升级文件失败
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////
MESPack MESRegRequest::toMESPack()
{
	//    FDAT_CMDSerial	1	BYTE	命令序列号:值固定为0x01
	//    FDAT_MACHINE_ID	20	ASC	机器ID
	//    FDAT_DEV_ID	20	ASC	终端ID
	//    FDAT_CommPassword	6	ASC	终端通讯密码
	//    FDAT_SystemType	20	ASC	当前系统类型和版本
	//    FDAT_SoftwareVer	20	ASC	软件版本号
	//    FDAT_FlashSpace	4	DWORD	Flash可用空间，单位字节
	//    FDAT_SystemTime	6	HEX	终端当前时间，若2012-06-20 09:18:32
	//    时间可以为：12,06,20,9,18,32
	//    FDAT_ConfigFileNum	1	HEX	配置文件个数M（0< M <= 10）
	//    FDAT_ConfigFileVer	26*M	HEX	共M组，每组内容包括：配置文件名称（20字节）+配置文件版本（最大允许10个文件的版本，每个版本6字节，用生成文件时间作为版本信息）
	//    FDAT_Reserved	10	HEX	保留

	MESPack pack(getCMDSerial()); //命令序列号：值固定为0x01
	pack.getBody().clear();
	QBuffer buffer(&pack.getBody());
	buffer.open(QIODevice::WriteOnly);
	QDataStream out(&buffer);
#define writeS(x)  out.writeRawData((char*)(x),sizeof(x))

	writeS(regInfo.FDAT_MACHINE_ID);
	writeS(regInfo.FDAT_DEV_ID);
	writeS(regInfo.FDAT_CommPassword);
	writeS(regInfo.FDAT_SystemType);
	writeS(regInfo.FDAT_SoftwareVer);
	out.writeRawData((char*) &regInfo.FDAT_FlashSpace, 4);
	writeS(regInfo.FDAT_SystemTime);
	out.writeRawData((char*) &regInfo.FDAT_ConfigFileNum, 1);
	writeS(regInfo.FDAT_Reserved);
#undef writeS
	buffer.close();
	return pack;
}

MESPack MESRegResponse::toMESPack()
{
	MESPack pack(0x01 + 0x80);
	return pack;
}

bool MESRegResponse::decode(MESPack &pack)
{
    memset(&echoInfo, 0, sizeof(echoInfo));

	if (pack.getCMDSerial() != 0x01 + 0x80)
		return false;    
	int versionCount = pack.getBody().length() - 96;
    if(!pack.CheckFlag())
    {
        Result=MESPack::FDAT_CHECK_SUM_ERR;
        return true;
    }
    QBuffer buffer(&pack.getBody());
	buffer.open(QIODevice::ReadOnly);
	QDataStream in(&buffer);
	in.readRawData(echoInfo.FDAT_MACHINE_ID, 20);
	in.readRawData(echoInfo.FDAT_DEV_ID, 20);
	in.readRawData((char*) &echoInfo.FDAT_PMSPermit, 1);
	//        00：终端状态正常，允许通讯
	//        01：IP地址不存在，拒绝通讯
	//        02：无效
	//        03：配置文件不完整
	in.readRawData((char*) &echoInfo.FDAT_LastSystemType, 20); //	ASC	最新的系统类型和版本
	in.readRawData((char*) &echoInfo.FDAT_LastSofrwareVer, 20); //	ASC	最新的软件版本号
	in.readRawData((char*) &echoInfo.FDAT_ServerSystemTime, 6); //	HEX	服务器系统时间
	if (versionCount > 0) //读版本
	{
		for (int i = 0; i < versionCount / 6; i++)
		{
			in.readRawData((char*) &echoInfo.FDAT_LastConfigFileVer[i], 6); //	HEX	服务器版本
		}
	}
	buffer.close();

	switch (echoInfo.FDAT_PMSPermit)
	{
	case 0x00:
		Result = MESPack::FDAT_RESULT_REG_OK;
		break;
	case 0x01:
		Result = MESPack::FDAT_RESULT_REG_IP_NON;
		break;
	case 0x02:
		Result = MESPack::FDAT_RESULT_REG_NON;
		break;
	case 0x03:
		Result = MESPack::FDAT_RESULT_REG_FILE_NON;
		break;
	default:
		Result = MESPack::FDAT_RESULT_NON;
		break;
	}

	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
//启动文件下载
MESPack MESFileDownRequest::toMESPack()
{
	MESPack pack(0x40);
//    FDAT_FileName	30	ASC	需要下载文件名字
//    FDAT_FileVer	10	ASC	本机现存文件版本号
//    FDAT_FlashSpace	4	DWORD	Flash可用空间，单位字节
	FDAT_FileVer.resize(10);
	pack.writeQString(this->FDAT_FileName, 30);
	pack.writeQByteArray(FDAT_FileVer);
	pack.writeChars((char*) &FDAT_FlashSpace, 4);
	return pack;
}

MESPack MESFileDownResponse::toMESPack()
{
	MESPack pack(0x40 + 0x80);
	return pack;
}
//启动文件下载响应
bool MESFileDownResponse::decode(MESPack &pack)
{
	if (pack.getCMDSerial() != 0x40 + 0x80)
		return false;
	//        FDAT_FileName	30	ASC	需要下载文件名字
	//        FDAT_LastFileVer	10	ASC	最新的文件版本号
	//        FDAT_LastFileSize	4	DWORD	最新的文件的大小
	//        FDAT_LastFileChecksum	4	DWORD	文件的CRC32校验值
	//        FDAT_Result	1	BYTE	反馈结果
    if(!pack.CheckFlag())
    {
        Result=MESPack::FDAT_CHECK_SUM_ERR;
        return true;
    }
	//        0x00：允许下载程序
	//        0x01：软件版本号相同，拒绝下载
	//        0x02：FLASH空间太小，不足够存放当前应用程序
	//        0x03：服务器忙，请稍后再下载
	//        0x04：其它未知错误
	pack.readToQString(this->FDAT_FileName, 30);
	pack.readToQByteArray(this->FDAT_LastFileVer, 10);
    FDAT_LastFileSize=pack.readQUint32();
    FDAT_LastFileChecksum=pack.readQUint32();
	this->FDAT_Result = pack.readQUint8();
	switch (FDAT_Result)
	{
	case 0x00:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_ALLOW;
		break;
	case 0x01:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_VERSION_CMP;
		break;
	case 0x02:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_SPACE_SMALL;
		break;
	case 0x03:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_SERVER_BUSY;
		break;
	case 0x04:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_OTHER;
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////
//文件流下载请求
MESPack MESFileDownStreamRequest::toMESPack()
{
//    FDAT_CMDSerial	1	BYTE	命令序列号：值固定为0x41
//    FDAT_FileName	30	ASC	需要下载文件名字
//    FDAT_OffsetAddr	4	DWORD	起始偏移地址
//    FDAT_PackSize	2	WORD	本次需要传输的包大小

	MESPack pack(0x41);

	pack.writeQString(this->FDAT_FileName, 30);
	pack.writeChars((char*) &FDAT_OffsetAddr, 4);
	pack.writeChars((char*) &FDAT_PackSize, 2);
	return pack;
}

//文件流下载响应
MESPack MESFileDownStreamResponse::toMESPack()
{
	MESPack pack(0x41 + 80);
	return pack;
}

bool MESFileDownStreamResponse::decode(MESPack &pack)
{
//    FDAT_CMDSerial	1	BYTE	命令序列号：值固定为(0x41 + 0x80)
//    FDAT_FileName	30	ASC	需要下载文件名字
//    FDAT_OffsetAddr	4	DWORD	起始偏移地址
//    FDAT_Data	N	HEX	数据包
//    FDAT_DataType	1	BYTE	数据类型类型
//    0x00：文件数据
//    0x01：该地址非法
//    0x02：包大小非法
//    0x03：其它未知错误
	if (pack.getCMDSerial() != 0x41 + 0x80)
		return false;
    if(!pack.CheckFlag())
    {
        Result=MESPack::FDAT_CHECK_SUM_ERR;
        return true;
    }
	pack.readToQString(FDAT_FileName, 30);
    FDAT_OffsetAddr=pack.readQUint32();
	pack.readToQByteArray(data, pack.getBody().length() - 30 - 4 - 1);
    FDAT_DataType=pack.readQUint8();

	switch (FDAT_DataType)
	{
	case 0x00:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_DATA;
		break;
	case 0x01:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_ADD_ERR;
		break;
	case 0x02:
		Result = MESPack::FDAT_RESULT_FILE_DOWN_PAK_ERR;
		break;
	}
	return true;
}
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
MESPack MESADJMachineRequest::toMESPack()
{
	MESPack pack(0x14);
//    FDAT_CMDSerial	1	BYTE	命令序列号：值固定为0x14
//    FDAT_DispatchNo	20	ASC	派工单号
//    FDAT_DispatchPrior	30	ASC	派工项次
//    FDAT_ProcCode	20	ASC	工序代码
//    FDAT_StaCode	10	ASC	站别代码
//    FDAT_StartCardID	10	ASC	开始调机卡号
//    FDAT_StartTime	6	HEX	开始调机时间
//    FDAT_EndCardID	10	ASC	结束调机卡号
//    FDAT_EndTime	6	HEX	结束调机时间
//    FDAT_CardType	1	HEX	调机原因编号
//    FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
	struct protocol_adj_machine
	{
		char FDAT_DispatchNo[20]; //	ASC	派工单号
		char FDAT_DispatchPrior[30]; //	ASC	派工项次
		char FDAT_ProcCode[20]; //	ASC	工序代码
		char FDAT_StaCode[10]; //	ASC	站别代码
		quint8 FDAT_StartCardID[10]; //	ASC	开始调机卡号
		quint8 FDAT_StartTime[6]; //	HEX	开始调机时间
		quint8 FDAT_EndCardID[10]; //	ASC	结束调机卡号
		quint8 FDAT_EndTime[6]; //	HEX	结束调机时间
		quint8 FDAT_CardType; //	HEX	调机原因编号
		quint8 FDAT_MultiNum;	//	HEX	总件数N（0< N <= 100）
	} info;

	struct protocol_adj_machine_item
	{
		char Adjust_ItemNO[20];	// ASC 产品编号
		quint32 Adjust_CurChangeQty;	// 4 DWORD 本次调机良品总数
		quint32 Adjust_NullQty;	// 4 DWORD 本次调机空模总数
		quint32 Adjust_ProdQty;	// 4 DWORD 本次调机生产总数
	} item;

	memset(&info, 0, sizeof(info));

	strcpy(info.FDAT_DispatchNo,
			this->adjData.getDispatchNo().toAscii().data()); //	ASC	派工单号
	strcpy(info.FDAT_DispatchPrior,
			this->adjData.getDispatchPrior().toAscii().data()); //	ASC	派工项次
	strcpy(info.FDAT_ProcCode, this->adjData.getProcCode().toAscii().data()); //	ASC	工序代码
	strcpy(info.FDAT_StaCode, this->adjData.getStaCode().toAscii().data()); //	ASC	站别代码
	MESTool::QStringToICCardID(this->adjData.getStartCardId(),
			info.FDAT_StartCardID); //	ASC	开始调机卡号
	MESTool::QStringTimToCharArray(this->adjData.getStartTime(),
			info.FDAT_StartTime); //	HEX	开始调机时间
	MESTool::QStringToICCardID(adjData.getEndCardId(), info.FDAT_EndCardID); //	ASC	结束调机卡号
	MESTool::QStringTimToCharArray(adjData.getEndTime(), info.FDAT_EndTime); //	HEX	结束调机时间
	info.FDAT_CardType = this->adjData.getCardType(); //	HEX	调机原因编号
	info.FDAT_MultiNum = this->adjData.getAdjstDataList().size(); //	HEX	总件数N（0< N <= 100）

	QByteArray &bufArry = pack.getBody();

	QBuffer buffer(&bufArry);
	buffer.open(QIODevice::WriteOnly);
	QDataStream stream(&buffer);

	stream.writeRawData((char*) info.FDAT_DispatchNo, 20);
	stream.writeRawData((char*) info.FDAT_DispatchPrior, 30);
	stream.writeRawData((char*) info.FDAT_ProcCode, 20);
	stream.writeRawData((char*) info.FDAT_StaCode, 10);
	stream.writeRawData((char*) info.FDAT_StartCardID, 10);
	stream.writeRawData((char*) info.FDAT_StartTime, 6);
	stream.writeRawData((char*) info.FDAT_EndCardID, 10);
	stream.writeRawData((char*) info.FDAT_EndTime, 6);
	stream.writeRawData((char*) &info.FDAT_CardType, 1);
	stream.writeRawData((char*) &info.FDAT_MultiNum, 1);

	for (int i = 0; i < info.FDAT_MultiNum; i++)
	{
		memset(&item, 0, sizeof(item));

		sprintf(item.Adjust_ItemNO,
				this->adjData.getAdjstDataList()[i].ItemNO.toAscii().data());
		item.Adjust_CurChangeQty =
				this->adjData.getAdjstDataList()[i].CurChangeQty;
		item.Adjust_NullQty = this->adjData.getAdjstDataList()[i].NullQty;
		item.Adjust_ProdQty = this->adjData.getAdjstDataList()[i].ProdQty;

		stream.writeRawData((char*) item.Adjust_ItemNO, 20); //	ASC	产品编号
		stream.writeRawData((char*) &item.Adjust_CurChangeQty, 4); //	DWORD	本次调机良品总数
		stream.writeRawData((char*) &item.Adjust_NullQty, 4); //	DWORD	本次调机空模总数
		stream.writeRawData((char*) &item.Adjust_ProdQty, 4); //	DWORD	本次调机生产总数
	}

	buffer.close();

	return pack;
}

////////////////////////////////////////////////////////////////////////
//控制命令请求与响应
MESPack MESControlRequest::toMESPack()
{
	MESPack pack(0xD7);
	return pack;
}

bool MESControlRequest::decode(MESPack &pack)
{
	if (pack.getCMDSerial() != 0xD7)
		return false;
	FDAT_DataType = pack.readQUint8();
	if (FDAT_DataType == 0x01)
		return true;
	pack.readToQByteArray(FDAT_CmdCode, 20);
	switch (FDAT_DataType)
	{
	case 0x00:   //设备复位
		break;
	case 0x01:   //读取设备RTC时间
		break;
	case 0x02:   //设置设备RTC时间
		rtcDateTime = pack.readQDateTime();
		break;
	case 0x03:   //读取设备剩余FLASH空间
		break;
	case 0x04:   //下载文件
		//30	ASC	需要下载的文件名
		//20	ASC/HEX	需要下载的文件版本号
		pack.readToQString(FDAT_FileName, 30);
		pack.readToQByteArray(FDAT_FileVer, 20);
		break;
	case 0x05:   //上传文件
		pack.readToQString(FDAT_FileName, 30);
		break;
	case 0x06:   //设置设备密码
		//FDAT_Data	10	ASC	设备的操作密码
		pack.readToQString(devPassword, 10);
		break;
	case 0x07:   //调整工单
//		FDAT_AdjustNo	20	ASC	调单流水号
//		FDAT_DispatchCount	1	HEX	工单总数（N）
//		FDAT_SerialNumber1	1	HEX	工单序号1
//		FDAT_DispatchNo1	20	ASC	派工单号1
//		FDAT_DispatchPrior1	30	ASC	派工项次1
//		...	...	...	...
//		FDAT_SerialNumberN	1	HEX	工单序号N
//		FDAT_DispatchNoN	20	ASC	派工单号N
//		FDAT_DispatchPriorN	30	ASC	派工项次N
	{
		quint8 FDAT_DispatchCount = 0;
		pack.readToQByteArray(FDAT_AdjustNo, 20);
		FDAT_DispatchCount = pack.readQUint8();
		orderIndexList.clear();
		for (int i = 0; i < FDAT_DispatchCount; i++)
		{
			OrderIndex index;
			index.FDAT_SerialNumber = pack.readQUint8();
			pack.readToQString(index.FDAT_DispatchNo, 20);
			pack.readToQString(index.FDAT_DispatchPrior, 30);
			orderIndexList.append(index);
		}
	}
		break;
	case 0x08:   //读取程序最新版本号
		break;
	case 0x09:   //设备换单
		break;
	case 0x0A:   //设置标准温度
		break;
	case 0x0B:   //设置SQLite语句
		//FDAT_ CommandLen	2	HEX	SQLite语句长度
		//FDAT_ CommandCont	N	ASC	命令内容
	{
		quint16 len = 0;
        len=pack.readQUint16();
		pack.readToQString(sql, len);
	}
		break;
	case 0x0C:   //配置文件初始化
		pack.readToQString(initFileName, 30);
		break;
	case 0x0D:   //设置班次
	{
		int count = pack.readQUint8();
		for (int i = 0; i < count; i++)
		{
			ClassInfo info;
			info.FDAT_SerialNumber = pack.readQUint8();
			info.FDAT_BeginTime[0] = pack.readQUint8();
			info.FDAT_BeginTime[1] = pack.readQUint8();
			info.FDAT_EndTime[0] = pack.readQUint8();
			info.FDAT_EndTime[1] = pack.readQUint8();
			classList.append(info);
		}
	}
		break;
	case 0x0E:
		pack.readToQString(this->FDAT_DispatchNo, 20);
		break;
	}
	return true;
}

void MESControlResponse::bindRequest(MESControlRequest &request)
{
	switch (request.FDAT_DataType)
	{
	case 0x00: // 0x00：设备复位
		break;
	case 0x01: // 0x01：读取设备RTC时间
		break;
	case 0x02: // 0x02：设置设备RTC时间
		break;
		//        0x03：读取设备剩余FLASH空间
	case 0x03:
		break;
		//        0x04: 下载文件
	case 0x04:
		break;
		//        0x05: 上传文件
	case 0x05:
		break;
		//        0x06: 设置设备密码
	case 0x06:
		break;
		//        0x07: 调整工单
	case 0x07:
		this->FDAT_AdjustNo = request.FDAT_AdjustNo;
		break;
		//        0x08: 读取程序最新版本号
	case 0x08:
		break;
		//        0x09: 设备换单
	case 0x09:
		break;
		//        0x0A: 设置标准温度
	case 0x0A:
		break;
		//        0x0B：设置SQLite语句
	case 0x0B:
		break;
		//        0x0C: 配置文件初始化
	case 0x0C:
		break;
		//        0x0D: 设置班次
	case 0x0D:
		break;
	default:
		return;
	}
}

MESPack MESControlResponse::toMESPack()
{
	MESPack pack(0x57);
	pack.writeQUint8(FDAT_DataType);
	pack.writeQUint8(FDAT_Result);
	pack.writeQByteArray(FDAT_CmdCode);
	switch (FDAT_DataType)
	{
	case 0x01: //0x01：读取设备RTC时间 响应
		pack.writeQDataTime(this->rtcTime);
		break;
	case 0x03: //        0x03：读取设备剩余FLASH空间
		pack.writeChars((char*) &flashBack, 4);
		break;
	case 0x07: 	//        0x07: 调整工单
		if (FDAT_AdjustNo.length() != 20)
		{
			FDAT_AdjustNo.resize(20);
			FDAT_AdjustNo.fill(0, 20);
		}
		pack.writeQByteArray(FDAT_AdjustNo);
		break;
	case 0x08: 	//0x08: 读取程序最新版本号
		if (FDAT_SoftwareVer.length() != 20)
		{
			FDAT_SoftwareVer.resize(20);
			FDAT_SoftwareVer.fill(0, 20);
		}
		pack.writeQByteArray(FDAT_SoftwareVer);
		break;
	case 0x0B:	//0x0B：设置SQLite语句
	{
		quint16 sqlDataLen = 0;
		sqlDataLen = sqliteData.length();
		pack.writeChars((char*) &sqlDataLen, 2);
		pack.writeQString(sqliteData, sqliteData.length());
	}
		break;
	}
	return pack;
}
/////////////////////////////////////////////////////////////////////////

bool MESADJMachineResponse::decode(MESPack &pack)
{
//    FDAT_CMDSerial	1	BYTE	命令序列号：值固定为(0x14 + 0x80)
//    FDAT_Result	1	BYTE	反馈接收结果
	if (pack.getCMDSerial() != 0x14 + 0x80)
		return false;
	int res = pack.readQUint8();
	Result = FDATResultFormatAppendix(res);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
MESBrushCardRequest::MESBrushCardRequest(const entity::BrushCard &data)
{
	memset(CardID, 0, sizeof(CardID));
	memset(CardDate, 0, sizeof(CardDate));

	this->DispatchNo = data.getDispatchNo(); //	20	ASC	派工单号
	this->DispatchPrior = data.getDispatchPrior(); //	30	ASC	派工项次
	this->ProcCode = data.getProcCode(); //	20	ASC	工序代码
	this->StaCode = data.getStaCode(); //	10	ASC	站别代码

	MESTool::QStringToICCardID(data.getCardId(), CardID); //	10	HEX	卡号
	this->CardType = data.getCardType(); //	1	HEX	刷卡原因编号
	MESTool::QStringTimToCharArray(data.getCardDate(), CardDate); //	6	HEX	刷卡数据产生时间
	this->IsBeginEnd = data.getIsBeginEnd(); //	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。

	switch (data.getCardType())
	{
	case entity::BrushCard::AsCONFIG_MANAGE: //		0	配置管理	进入配置管理界面，不可配置
	case entity::BrushCard::AsQC_NORMAL_CHECK: //	1	QC巡检	默认一定存在，不需要配置（次品录入功能），不可配置
	case entity::BrushCard::AsCHANGE_MOLD: //21  换模
	case entity::BrushCard::AsCHANGE_MATERIAL: //22  换料
	case entity::BrushCard::AsCHANGE_ORDER: //23  换单
	case entity::BrushCard::AsWAITTING_MATERIAL: //27  待料
	case entity::BrushCard::AsMAINTAIN: //28  保养
	case entity::BrushCard::AsWAITTING_PERSON: //29  待人
	case entity::BrushCard::AsHANDOVER_DUTY: //30  交接班刷卡
	case entity::BrushCard::AsMATERIAL_BAD: //31  原材料不良
	case entity::BrushCard::AsPLANNED_DOWNTIME: //32  计划停机
	case entity::BrushCard::AsOFF_DUTY: //34 下班
	case entity::BrushCard::AsDEBUG_MACHINE: //36  调机
	case entity::BrushCard::AsORDER_ADJUST: //37  调单
	case entity::BrushCard::AsSOCKET_NUM: //38  调整模穴数
	case entity::BrushCard::AsENGINEERING_WAIT: //39  工程等待
	case entity::BrushCard::AsADD_MATERIAL: //40  添料
	case entity::BrushCard::AsINSPECT_PRO: //41  巡机
	case entity::BrushCard::AsPOLISH_PRO: //42  打磨
	case entity::BrushCard::AsMOLD_TESTING: //44  试模
	case entity::BrushCard::AsDEVICE_CHECK_ITEM: //45  点检
	case entity::BrushCard::AsPO_ELECTRIC_QUANTITY: //46  用电量
	case entity::BrushCard::AsTest_Material: //47  试料
	case entity::BrushCard::AsQUALITY_CHECK: //48  品质检验
		Body.clear();
		break;
	case entity::BrushCard::AsCHANGE_STANDARD_CYCLE: //35  修改周期
	{	//31、	修改周期携带数据格式
		//CC_StandCycle	4	HEX	新标准周期，毫秒
		entity::BrushCard::CardStandCycle cardstandCycle;
		entity::XMLToPropertyBase(data.getCarryData(), cardstandCycle);
		Body.append((char*) &cardstandCycle.StandCycle, 4);
		break;
	}
	case entity::BrushCard::AsORDER_ALLOT:  			//43  工单调拔
	{	//32、	工单调拨携带数据格式（CC_CarryData）
//CC_MachineNo	20	ASC	新机器编号
		entity::BrushCard::CardMachineNo cardMachineNo;
		entity::XMLToPropertyBase(data.getCarryData(), cardMachineNo);
		Body.resize(20);
		Body.fill(0, 20);
		Body.replace(0, cardMachineNo.MachineNo.length(),
				cardMachineNo.MachineNo.toAscii().data());
		break;
	}
		//33、	机器故障、模具故障、辅设故障刷开始卡携带数据格式（CC_CarryData）
		//34、	机器故障、模具故障、辅设故障刷结束卡携带数据格式（CC_CarryData）
	case entity::BrushCard::AsAUXILIARY_FAILURE:		//24  辅设故障
	case entity::BrushCard::AsMACHINE_FAILURE:		//25  机器故障
	case entity::BrushCard::AsMOLD_FAILURE:		//26  模具故障
	{
		if (data.getIsBeginEnd() == 0) //开始
		{
			if (data.getCarryData().length() > 0)
			{
				entity::BrushCard::CardFault faultStart;
				entity::XMLToPropertyBase(data.getCarryData(), faultStart);
				//ResultNo==0 则代表 设备保障,否则代表 预计维修时间
				if (faultStart.ResultNo == 0)
				{
					Body.append((char) (quint8) faultStart.FaultNo);
				}
				else
				{
					Body.append((char) (quint8) faultStart.FaultNo);
					Body.append((char*) &faultStart.ResultNo, 4);
				}
			}
			else
			{
				Body.clear();
			}
		}
		else
		{
			if (data.getCarryData().length() > 0)
			{
				entity::BrushCard::CardFault start;
				entity::XMLToPropertyBase(data.getCarryData(), start);

				Body.append((char) (quint8) start.FaultNo);
				Body.append((char) (quint8) start.ResultNo); //维修结果序号 NG OK
			}
			else
			{
				Body.clear();
			}
		}
		break;
	}
	case entity::BrushCard::AsQC_FAULT_REPAIR: //2	故障维修	功能界面不显示，只做维修卡上报
	{	//35、	机器维修数据格式（CC_CarryData）
		//CC_FaultNo	20	HEX	故障识别码
		entity::BrushCard::CardFaultNo cardFaultNo;
		entity::XMLToPropertyBase(data.getCarryData(), cardFaultNo);
		Body.resize(20);
		Body.fill(0, 20);
		Body.replace(0, cardFaultNo.FaultNo.length(),
				cardFaultNo.FaultNo.toAscii().data());
		break;
	}
	case entity::BrushCard::AsON_DUTY:					//33 上班
	{	//36、	上班携带数据格式（CC_CarryData）
//CC_WorkType	1	HEX	员工的工作类别
		entity::BrushCard::CardUpWork work;
		entity::XMLToPropertyBase(data.getCarryData(), work);
		Body.append((char) (quint8) work.WorkType);
		break;
	}
	default:
		break;
	}
}

MESPack MESBrushCardRequest::toMESPack()
{
	MESPack pack(this->getCMDSerial());
	pack.writeQUint8(0x00);//刷卡数据类型
	//0x00：标准刷卡数据
	pack.writeQString(DispatchNo, 20);	//	ASC	派工单号
	pack.writeQString(DispatchPrior, 30);	//	ASC	派工项次
	pack.writeQString(ProcCode, 20);	//	ASC	工序代码
	pack.writeQString(StaCode, 10);	//	ASC	站别代码
    pack.writeChars((char*) CardID,10);	//	HEX	卡号
	pack.writeChars((char*) &CardType, 1);	//	HEX	刷卡原因编号
	pack.writeChars((char*) CardDate, 6);	//	HEX	刷卡数据产生时间
	pack.writeChars((char*) &IsBeginEnd, 1);	//	HEX	刷卡开始或结束标记，0表示开始，1表示结束。
	quint16 CC_CarryDataLen = Body.length();	//	2	HEX	刷卡携带数据长度（N）
	pack.writeChars((char*) &CC_CarryDataLen, 2);
	pack.writeQByteArray(Body);
	return pack;
}

//////////////////////////////////////////////////////////////////
MESDefectiveInfoRequest::MESDefectiveInfoRequest(entity::DefectiveInfo &info)
{
	this->data = info;
}

MESPack MESDefectiveInfoRequest::toMESPack()
{
	MESPack pack(this->getCMDSerial());

    pack.writeQString(data.getDispatchNo(), 20);//	20	ASC	派工单号
    pack.writeQString(data.getDispatchPrior(), 30);//	30	ASC	派工项次
    pack.writeQString(data.getProcCode(), 20);	//	20	ASC	工序代码
    pack.writeQString(data.getStaCode(), 10);	//	10	ASC	站别代码
	pack.writeQStringIC(data.getCardId());	//	10	HEX	卡号
	pack.writeQDataTime(MESTool::QStringToDateTime(data.getCardDate()));//	6	HEX	次品或隔离品数据产生时间
	pack.writeQUint8(data.getStatus());	//	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
	pack.writeQUint8(data.getBadDataList().size());	//	1	HEX	总件数N（0< N <= 100）
    pack.writeQUint8(data.getBadReasonNum());//	1	HEX	次品或隔离品原因总数M（0< M <= 100）
    //pack.writeQUint8(data.getFdatBillType());	//	1	HEX	单据类别(0表示次品,1表示隔离品) 没有隔离品

	for (int i = 0; i < data.getBadDataList().size(); i++)
	{
		entity::DefectiveInfo::ProductInfo &productInfo =
				data.getBadDataList()[i];
		//Bad_ItemNo	20	ASC	产品编号
		//Bad_BadQty	4	DWORD	次品或隔离品总数

		pack.writeQString(productInfo.getItemNo(), 20);        // 20	ASC	产品编号
		pack.writeQUint32(productInfo.getBadQty());

		for (int j = 0; j < productInfo.getBadData().size(); j++)
		{
            int v = productInfo.getBadData()[j];
			pack.writeQUint32(v);
		}
	}
	return pack;
}

////////////////////////////////////////////////////////////////////////////
MESOtherRequest::MESOtherRequest(entity::OtherSetInfo &data)
{
	pack.setCMDSerial(this->getCMDSerial());
	pack.writeQUint8((quint8) data.getDataType());

	switch (data.getDataType())
	{
	case entity::OtherSetInfo::AsUpgradeFile: // = 0x00, //：升级文件
	{
//        FDAT_FileName	30	ASC	升级文件名
//        FDAT_Result	1	BYTE	反馈结果
		entity::OtherSetInfo::UpgradeFile file;
		data.toUpgradeFile(file);
		pack.writeQString(file.FileName, 30);
		pack.writeQUint8(file.Result);
		//发送到服务器
	}
		break;
	case entity::OtherSetInfo::AsAsyncOrder: // = 0x01,//：同步工单 与界面相关
	{

	}
		break;
	case entity::OtherSetInfo::AsAdjOrder: // = 0x02,//：调整工单
	{
		QList<entity::OtherSetInfo::AdjOrder> info;
		data.toAdjOrderList(info);
//        FDAT_DispatchCount	1	HEX	工单总数（N）
//        FDAT_SerialNumber1	1	HEX	工单序号1
//        FDAT_DispatchNo1	20	ASC	派工单号1
//        FDAT_DispatchPrior1	30	ASC	派工项次1
		pack.writeQUint8(info.size());
		for (int i = 0; i < info.size(); i++)
		{
			pack.writeQUint8(info[i].SerialNumber);
			pack.writeQString(info[i].DispatchNo, 20);
			pack.writeQString(info[i].DispatchPrior, 30);
		}
	}
		break;
	case entity::OtherSetInfo::AsAdjSock: // = 0x03,//：调整模穴数
	{
		entity::OtherSetInfo::AdjSock sock;
		data.toAdjSock(sock);
//        FDAT_DispatchNo	20	ASC	派工单号
//        FDAT_DispatchPrior	30	ASC	派工项次
//        FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
//        PCS_MO	20	ASC	工单号
//        PCS_ItemNO	20	ASC	产品编号
//        PCS_DispatchQty	4	DWORD	派工数量
//        PCS_SocketNum	1	HEX	模穴数
		pack.writeQString(sock.DispatchNo, 20);
		pack.writeQString(sock.DispatchPrior, 30);
		pack.writeQUint8(sock.productList.size());

		for (int i = 0; i < sock.productList.size(); i++)
		{
			pack.writeQString(sock.productList[i].MO, 20);
			pack.writeQString(sock.productList[i].ItemNO, 20);
			pack.writeQUint32(sock.productList[i].DispatchQty);
			pack.writeQUint8(sock.productList[i].SocketNum);
		}
	}
		break;
	case entity::OtherSetInfo::AsAdjMaterial: // = 0x04,//：调整原料
	{
		entity::OtherSetInfo::AdjMaterial info;
		data.toAdjMaterial(info);
//        FDAT_MaterialCount	1	HEX	原料总数（N）
//        FDAT_DispatchNo	20	ASC	派工单号
//        FDAT_CardID	10	ASC	员工卡号
//        FDAT_MaterialNO1	50	ASC	原料1编号
//        FDAT_MaterialName1	100	ASC	原料1名称
//        FDAT_ BatchNO1	50	ASC	原料1批次号
//        FDAT_ FeedingQty1	4	HEX	原料1投料数量
//        FDAT_ FeedingTime1	6	HEX	原料1投料时间
		pack.writeQUint8(info.MaterialList.size());
		pack.writeQString(info.DispatchNo, 20);
		pack.writeQStringIC(info.CardID);

		for (int i = 0; i < info.MaterialList.size(); i++)
		{
			pack.writeQString(info.MaterialList[i].BatchNO, 50);
			pack.writeQString(info.MaterialList[i].MaterialName, 100);
			pack.writeQString(info.MaterialList[i].BatchNO, 50);
			pack.writeQUint32(info.MaterialList[i].FeedingQty);
			pack.writeQDataTime(
					MESTool::QStringToDateTime(
							info.MaterialList[i].FeedingTime));
		}
	}
		break;
	case entity::OtherSetInfo::AsCardStopNo: // = 0x05,//：刷完停机卡计数
	{
		entity::OtherSetInfo::CardStopNo info;
		data.toCardStopNo(info);
//        CC_DispatchNo	20	ASC	派工单号
//        CC_DispatchPrior	30	ASC	派工项次
//        CC_ProcCode	20	ASC	工序代码
//        CC_StaCode	10	ASC	站别代码
//        CC_CardType	1	HEX	刷卡原因编号
//        CC_CardDate	6	HEX	生产时间
		pack.writeQString(info.DispatchNo, 20);
		pack.writeQString(info.DispatchPrior, 30);
		pack.writeQString(info.ProcCode, 20);
		pack.writeQString(info.StaCode, 20);
		pack.writeQUint8(info.CardType);
		pack.writeQDataTime(MESTool::QStringToDateTime(info.CardDate));
	}
		break;
	case entity::OtherSetInfo::AsDevInspection: // = 0x06,//：设备点检
	{
		entity::OtherSetInfo::DevInspection info;
		data.toDevInspection(info);
//        CC_DispatchNo	20	ASC	派工单号
//        CC_DispatchPrior	30	ASC	派工项次
//        CC_ProcCode	20	ASC	工序代码
//        CC_StaCode	10	ASC	站别代码
//        CC_CardID	10	HEX	卡号
//        CC_CardDate	6	HEX	点检时间
//        CC_InspectNum	1	HEX	点检总数N（0< N <= 100）
//---------------------------------------
//        PCS_NO	1	HEX	点检项目编号
//        PCS_Result	1	HEX	点检结果1:OK,0:NG
//        PCS_Brand	30	ASC	机器品牌
		pack.writeQString(info.DispatchNo, 20);
		pack.writeQString(info.DispatchPrior, 30);
		pack.writeQString(info.ProcCode, 20);
		pack.writeQString(info.StaCode, 10);
		pack.writeQStringIC(info.CardID);
		pack.writeQDataTime(MESTool::QStringToDateTime(info.CardDate));
		pack.writeQUint8(info.nDataList.size());
		for (int i = 0; i < info.nDataList.size(); i++)
		{
			pack.writeQUint8(info.nDataList[i].NO);
			pack.writeQUint8(info.nDataList[i].Result);
			pack.writeQString(info.nDataList[i].Brand, 30);
		}
	}
		break;
	case entity::OtherSetInfo::AsELEFeeCount: // = 0x07 电费统计
	{
		entity::OtherSetInfo::ELEFeeCount info;
		data.toELEFeeCount(info);

//        CC_DispatchNo	20	ASC	派工单号
//        CC_DispatchPrior	30	ASC	派工项次
//        CC_ProcCode	20	ASC	工序代码
//        CC_StaCode	10	ASC	站别代码
//        CC_CardID	10	HEX	录入人卡号
//        CC_CardDate	6	HEX	录入时间
//        CC_ElecNum	4	HEX	当前班次电费
		pack.writeQString(info.DispatchNo, 20);
		pack.writeQString(info.DispatchPrior, 30);
		pack.writeQString(info.ProcCode, 20);
		pack.writeQStringIC(info.CardID);
		pack.writeQDataTime(MESTool::QStringToDateTime(info.CardDate));
		pack.writeQUint32(info.ElecNum);
	}
		break;
	}
}

MESPack MESOtherRequest::toMESPack()
{
	return pack;
}

MESOtherRequest::MESOtherRequest()
{
}

void MESOtherRequest::setAsyncOrder(QStringList &orderList)
{
	pack.setCMDSerial(this->getCMDSerial());
	pack.writeQUint8((quint8) 0x01); //：同步工单 与界面相关
	pack.writeQUint8(orderList.count()); //工单总数
	for (int i = 0; i < orderList.count(); i++)
		pack.writeQString(orderList[i], 20); //派工单号
}

bool MESOtherResponse::decode(MESPack &pack)
{
	if (pack.getCMDSerial() != 0x15 + 0x80)
		return false;
	int FDAT_Result = 0;
	FDAT_DataType = (entity::OtherSetInfo::SetType) pack.readQUint8();
	FDAT_Result = pack.readQUint8();
	Result = MESPack::FDAT_RESULT_SUCESS;
	switch (FDAT_DataType)
	{
	case entity::OtherSetInfo::AsUpgradeFile: // = 0x00, //：升级文件
	{
		pack.readToQString(FDAT_FileName, 30);
		break;
	}
	case entity::OtherSetInfo::AsAsyncOrder: // = 0x01, //：同步工单
	{
//        FDAT_DispatchCount	1	HEX	工单总数（N）
//        FDAT_SerialNumber1	1	HEX	工单序号1
//        FDAT_DispatchNo1	20	ASC	派工单号1
//        ...	...	...	...
//        FDAT_SerialNumberN	1	HEX	工单序号N
//        FDAT_DispatchNoN	20	ASC	派工单号N
		int count = pack.readQUint8();
		for (int i = 0; i < count; i++)
		{
			OrderIndex index;
			index.FDAT_SerialNumber = pack.readQUint8();
			pack.readToQString(index.FDAT_DispatchNo, 20);
			orderIndexList.append(index);
		}
		break;
	}
	case entity::OtherSetInfo::AsAdjOrder: // = 0x02, //：调整工单
	case entity::OtherSetInfo::AsAdjSock: // = 0x03, //：调整模穴数
	case entity::OtherSetInfo::AsAdjMaterial: // = 0x04, //：调整原料
	case entity::OtherSetInfo::AsCardStopNo: // = 0x05, //：刷完停机卡计数
	case entity::OtherSetInfo::AsDevInspection: // = 0x06, //：设备点检
	case entity::OtherSetInfo::AsELEFeeCount: // = 0x07 //：电费统计
		break;
	}
	return true;
}
////////////////////////////////////////////////////////////////

MESQualityRegulateRequest::MESQualityRegulateRequest(
		entity::QualityRegulate &info)
{
//    FDAT_DispatchNo	20	ASC	派工单号
//    FDAT_DispatchPrior	30	ASC	派工项次
//    FDAT_ProcCode	20	ASC	工序代码
//    FDAT_StaCode	10	ASC	站别代码
//    FDAT_StartCardID	10	ASC	开始卡号
//    FDAT_StartTime	6	HEX	开始时间
//    FDAT_EndCardID	10	ASC	结束卡号
//    FDAT_EndTime	6	HEX	结束时间
//    FDAT_CardType	1	HEX	刷卡原因编号:巡机、打磨等
//    FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
//---------------------------------------------------
//    Func_ItemNO	20	ASC	产品编号
//    Func_CurChangeQty	4	DWORD	本次记录总数
	pack.setCMDSerial(this->getCMDSerial());
	pack.writeQString(info.getDispatchNo(), 20);
	pack.writeQString(info.getDispatchPrior(), 30);
	pack.writeQString(info.getProcCode(), 20);
	pack.writeQString(info.getStaCode(), 10);
	pack.writeQStringIC(info.getStartCardNo());
	pack.writeQDataTime(MESTool::QStringToDateTime(info.getStartTime()));
	pack.writeQStringIC(info.getEndCardNo());
	pack.writeQDataTime(MESTool::QStringToDateTime(info.getEndTime()));
	pack.writeQUint8(info.getCardType());
	pack.writeQUint8(info.getDataList().size());
	for (int i = 0; i < info.getDataList().size(); i++)
	{
		pack.writeQString(info.getDataList()[i].ItemNO, 20);
		pack.writeQUint32(info.getDataList()[i].CurChangeQty);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
MESProduceRequest::MESProduceRequest(entity::Producted &producted)
{
	//	FDAT_DataType	1	BYTE	生产数据类型
	//	0x00：标准注塑机生产数据
	//	其他：待定
	pack.setCMDSerial(this->getCMDSerial());
	pack.writeQUint8(0x00);


//    PD_DispatchNo	20	ASC	派工单号
//    PD_DispatchPrior	30	ASC	派工项次
//    PD_ProcCode	20	ASC	工序代码
//    PD_StaCode	10	ASC	站别代码
//    PD_EndCycle	6	HEX	生产数据产生时间（成型周期结束时间）
//    PD_TemperValue	2*N	HEX	N路温度值，每路2字节（依序为温度1、温度2、……、温度N）
//    PD_MachineCycle	4	HEX	机器周期，毫秒
//    PD_FillTime	4	HEX	填充时间，毫秒
//    PD_CycleTime	4	HEX	成型周期，毫秒
//    PD_TotalNum	4	HEX	模次（成模次序，原啤数）
//    PD_KeepPress	2*100	HEX	100个压力点
//    PD_StartCycle	6	HEX	生产数据开始时间（成型周期开始时间）
	pack.writeQString(producted.getDispatchNo(), 20); //	20	ASC	派工单号
	pack.writeQString(producted.getDispatchPrior(), 30); //	30	ASC	派工项次
	pack.writeQString(producted.getProcCode(), 20);  //	20	ASC	工序代码
	pack.writeQString(producted.getStaCode(), 10); //	10	ASC	站别代码
	pack.writeQDataTime(MESTool::QStringToDateTime(producted.getStartCycle())); //	6	HEX	生产数据产生时间（成型周期结束时间）

	for (int i = 0; i < 8; i++) //N路温度值
	{
		if (i < producted.getKeepPress().size())
			pack.writeQUint16(producted.getTemperValue()[i]);
		else
			pack.writeQUint16(0);
	}

	pack.writeQUint32(producted.getMachineCycle());
	pack.writeQUint32(producted.getFillTime());
	pack.writeQUint32(producted.getCycleTime());
	pack.writeQUint32(producted.getTotalNum());

	for (int i = 0; i < 100; i++)
	{
		if (i < producted.getKeepPress().size())
			pack.writeQUint16(producted.getKeepPress().at(i));
		else
			pack.writeQUint16(0);
	}

}

bool MESOrderRequest::decode(MESPack& pack)
{
	if (pack.getCMDSerial() != getCMDSerial())
		return false;
	FDAT_DataType = pack.readQUint16();
	QString MO_DispatchNo; //	20	ASC	派工单号
	QString MO_DispatchPrior; //	30	ASC	派工项次
	QString MO_ProcCode; //	20	ASC	工序代码
	QString MO_ProcName; //	50	ASC	工序名称
	QString MO_StaCode; //	10	ASC	站别代码
	QString MO_StaName; //	20	ASC	站别名称（工作中心）
    qint32 MO_StandCycle; //	4	DWORD	标准周期，毫秒
    qint32 MO_TotalNum; //	4	DWORD	模次,生产次数（原啤数）
	int MO_MultiNum; //	1	HEX	总件数N（0< N <= 100）
	int MO_BadTypeNo; //	1	HEX	次品类型选项（配置文件中）
	int MO_BadReasonNum; //	1	HEX	次品原因总数M（0< M <= 100）

	pack.readToQString(MO_DispatchNo, 20); //MO_DispatchNo	20	ASC	派工单号
	pack.readToQString(MO_DispatchPrior, 30); //MO_DispatchPrior	30	ASC	派工项次
	pack.readToQString(MO_ProcCode, 20); //MO_ProcCode	20	ASC	工序代码
	pack.readToQString(MO_ProcName, 50); //MO_ProcName	50	ASC	工序名称
	pack.readToQString(MO_StaCode, 10); //MO_StaCode	10	ASC	站别代码
	pack.readToQString(MO_StaName, 20); //MO_StaName	20	ASC	站别名称（工作中心）
    MO_StandCycle=pack.readQUint32(); //MO_StandCycle	4	DWORD	标准周期，毫秒
    MO_TotalNum=pack.readQUint32(); //MO_TotalNum	4	DWORD	模次,生产次数（原啤数）
	MO_MultiNum = pack.readQUint8(); //MO_MultiNum	1	HEX	总件数N（0< N <= 100）
	MO_BadTypeNo = pack.readQUint8(); //MO_BadTypeNo	1	HEX	次品类型选项（配置文件中）
	MO_BadReasonNum = pack.readQUint8(); //MO_BadReasonNum	1	HEX	次品原因总数M（0< M <= 100）
	order.setMoDispatchNo(MO_DispatchNo);
	order.setMoDispatchPrior(MO_DispatchPrior);
	order.setMoProcCode(MO_ProcCode);
	order.setMoProcName(MO_ProcName);
	order.setMoStaCode(MO_StaCode);
	order.setMoStaName(MO_StaName);
	order.setMoStandCycle(MO_StandCycle);
    order.setMoTotalNum(MO_TotalNum);
	order.setMoMultiNum(MO_MultiNum);
	order.setMoBadTypeNo(MO_BadTypeNo);
    order.setMoBadReasonNum(MO_BadReasonNum);

    logDebug(QString("派工单号:%1").arg(MO_DispatchNo));
    logDebug(QString("派工项次:%1").arg(MO_DispatchPrior));
    logDebug(QString("工序代码:%1").arg(MO_ProcCode));
    logDebug(QString("工序名称:%1").arg(MO_ProcName));
    logDebug(QString("站别代码:%1").arg(MO_StaCode));
    logDebug(QString("站别名称（工作中心）:%1").arg(MO_StaName));
    logDebug(QString("标准周期，毫秒:%1").arg(MO_StandCycle));
    logDebug(QString("模次:%1").arg(MO_TotalNum));

	//26、	每件产品的数据格式PCS_Data
	for (int i = 0; i < order.getMoMultiNum(); i++)
	{
		entity::OrderBoy boy;
		QString PCS_MO;
		QString PCS_ItemNO;
		QString PCS_ItemName;
		QString PCS_MouldNo;
		pack.readToQString(PCS_MO, 20);	//PCS_MO	20	ASC	工单号
		pack.readToQString(PCS_ItemNO, 20);	//PCS_ItemNO	20	ASC	产品编号
		pack.readToQString(PCS_ItemName, 50);	//PCS_ItemName	50	ASC	产品描述
		pack.readToQString(PCS_MouldNo, 20);	//PCS_MouldNo	20	ASC	模具编号
		boy.setPcsMo(PCS_MO);
		boy.setPcsItemNo(PCS_ItemNO);
		boy.setPcsItemName(PCS_ItemName);
		boy.setPcsMouldNo(PCS_MouldNo);

		boy.setPcsDispatchQty(pack.readQUint32());//PCS_DispatchQty	4	DWORD	派工数量
		boy.setPcsSocketNum1(pack.readQUint8());//PCS_SocketNum1	1	HEX	模具可用模穴数
		boy.setPcsSocketNum2(pack.readQUint8());//PCS_SocketNum2	1	HEX	产品可用模穴数
		boy.setPcsFitMachineMin(pack.readQUint32());//PCS_ FitMachineMin	4	HEX	模具适应机型吨位最小值
		boy.setPcsFitMachineMax(pack.readQUint32());//PCS_ FitMachineMax	4	HEX	模具适应机型吨位最大值
		boy.setPcsBadQty(pack.readQUint32());	//PCS_BadQty	4	DWORD	次品总数
		//每件产品的次品数据
		for (int j = 0; j < order.getMoBadReasonNum(); j++)
			boy.getPcsBadData().append(pack.readQUint32());
        boy.setPcsAdjNum(pack.readQUint32()); //调机数

        // 计算已生产良品数 模次*产品模穴+调机数-次品总数
        boy.setTotalOkNum( MO_TotalNum * boy.getPcsSocketNum2()+boy.getPcsAdjNum()-boy.getPcsBadQty());
		order.getBoy().append(boy);
	}
	//原料总数（精美客户提出）
	//此工单所用原料信息（精美客户提出）
	if (!pack.isReadEnd())
	{
		int matCount = pack.readQUint8();
		for (int i = 0; i < matCount; i++)
		{
			entity::Material material;
			QString MaterialNO, MO_MaterialName;
			pack.readToQString(MaterialNO, 50);	//MO_MaterialNO1	50	ASC	原料1编号
			pack.readToQString(MO_MaterialName, 100);//MO_MaterialName1	100	ASC	原料1名称

			material.setMaterialNo(MaterialNO);
			material.setMaterialName(MO_MaterialName);
			material.setDispatchNo(order.getMoDispatchNo());
			MaterialList.append(material);
		}
	}
	return true;
}
//////////////////////////////////////////////////////////////
bool MESNoticeRequest::decode(MESPack& pack)
{
	if (pack.getCMDSerial() != 0xD8)
		return false;
	StartTime = pack.readQDateTime();	//FDAT_StartDate	6	HEX	公告有效开始时间
	EndTime = pack.readQDateTime();	//FDAT_EndDate	6	HEX	公告有效结束时间
	quint16 FDAT_NoticeLen = pack.readQUint16();//FDAT_NoticeLen	2	HEX	公告数据长度
	//FDAT_Data	N	HEX	根据FDAT_NoticeLen决定本数据段的长度(N < 512 ,最多只允许250个汉字)
	pack.readToQString(Text, FDAT_NoticeLen);
	return true;
}
