/*
 * DigitalMeterDl64597.cpp
 *
 *  Created on: 2015年6月2日
 *      Author: cj
 */

#include "digitalmeterdl64597.h"

bool DigitalMeter_Dl64597::ReadFlag;

DigitalMeter_Dl64597::DigitalMeter_Dl64597()
	  :Masterserial(0)
{
	// TODO Auto-generated constructor stub
	DigitalMeter_Dl64597::ReadFlag = false;
}

DigitalMeter_Dl64597::~DigitalMeter_Dl64597()
{
	// TODO Auto-generated destructor stub
}

bool DigitalMeter_Dl64597::DigitalMeter_send_read_ele_frame()
{
	quint8 ucCnt = 18;
	quint8 chSendbuf[20] = "\xFE\xFE\xFE\xFE\x68\xAA\xAA\xAA\xAA\xAA\xAA\x68\x01\x02\x43\xC3\xD5\x16";
	quint8 chSendBufT[20] = {0xFE,0xFE,0xFE,0xFE,0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x01,0x02,0x43,0xC3,0xD5,0x16};
	const quint8 chSendBufT2[20] = {0xFE,0xFE,0xFE,0xFE,0x68,0x99,0x99,0x99,0x99,0x99,0x99,0x68,0x01,0x02,0x43,0xC3,0x6F,0x16};
	if(Masterserial->sendData((char*)chSendBufT2, ucCnt) != ucCnt)
	{
		return false;
	}
	return true;
}

int DigitalMeter_Dl64597::DigitalMeter_read_data(quint32* data)
{
	int len = 100;
	return Masterserial->readData((char*)data,len);
}

bool DigitalMeter_Dl64597::getReadMeterFlag()
{
	return ReadFlag;
}
void DigitalMeter_Dl64597:: PostElectMeter()
{
   DigitalMeter_Dl64597::ReadFlag =true;
}
void DigitalMeter_Dl64597::setSerialHandle( Serial* fd)
{
	Masterserial = fd;
	if(Masterserial != 0)
	{
		Masterserial->init(B_115200_BITERATE,8,1,'N');
	}
}

bool DigitalMeter_Dl64597::saveMasterData(quint32 elecnum)
{
	// 产生用电量数据
		quint16 uwDataCnt = 0;
		quint8 Send_buf[3*1024];
		quint16 Send_len=0;
		quint8 ucCardNo[4] = {0,0,0,1};
		memset(Send_buf,0,sizeof(Send_buf));
		Send_buf[0]=0x15; //刷卡数据命令头  发送其它设置信息
		uwDataCnt++;
		Send_buf[1]=0x07; // 0x07 用电量
		uwDataCnt++;


		//Order order;
		//Order::query(order, 1);
		OtherSetInfo::ELEFeeCount info;
		info.DispatchNo = OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo();
		info.DispatchPrior = OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchPrior();
		info.ProcCode = OrderMainOperation::GetInstance().mainOrderCache.getMoProcCode();
		info.CardID = "0001";
		info.CardDate = Tool::GetCurrentDateTimeStr();
		info.ElecNum = elecnum;

		OtherSetInfo otherInfo;
		otherInfo.setELEFeeCount(info);
		Notebook("耗电量录入", otherInfo).insert();


		DigitalMeter_Dl64597::ReadFlag = false;
        return true;

//		// 派工单号20B
//		memcpy(Send_buf + uwDataCnt,g_tMainOrderInfo.tFixedOrderInfo.cDispatchNo,  DISPATCH_NO_SIZE);
//		uwDataCnt += DISPATCH_NO_SIZE;
//		// 派工项次30B
//		memcpy(Send_buf + uwDataCnt,g_tMainOrderInfo.tFixedOrderInfo.cDispatchPrior,  DISPATCH_PRIOR_SIZE);
//		uwDataCnt += DISPATCH_PRIOR_SIZE;
//		// 工序代码20B
//		memcpy(Send_buf + uwDataCnt,g_tMainOrderInfo.tFixedOrderInfo.cProcCode, PROC_CODE_SIZE);
//		uwDataCnt += PROC_CODE_SIZE;
//		// 站别代码10B
//		memcpy(Send_buf + uwDataCnt,g_tMainOrderInfo.tFixedOrderInfo.cStaCode, STA_CODE_SIZE);
//		uwDataCnt += STA_CODE_SIZE;
//		//卡号
//		//Send_buf[uwDataCnt+3]=0x01;
//		memcpy(Send_buf + uwDataCnt,(void *)&ucCardNo,sizeof(ucCardNo));
//		uwDataCnt += 10;
//
//		//刷卡时间
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwYear;
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwMonth;
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwDay;
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwHour;
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwMinute;
//		Send_buf[uwDataCnt++]=(u8)g_tDevVar.tCurSysTime.uwSecond;
//
//		//用电量
//		Send_buf[uwDataCnt++] = (u8)(udwElectric);
//		Send_buf[uwDataCnt++] = (u8)(udwElectric >> 8);
//		Send_buf[uwDataCnt++] = (u8)(udwElectric >> 16);
//		Send_buf[uwDataCnt++] = (u8)(udwElectric >> 24);
//
//		Send_len=uwDataCnt;
}
