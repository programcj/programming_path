/*
 * seriallister.cpp
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */

#include <QtDebug>
#include "../Public/public.h"
#include "rs485lister.h"
#include <QFile>
#ifndef _WIN32
#include <fcntl.h>
#endif
#define max_recv_buffer  (4*1024)

Rs485Lister::Rs485Lister()
{
#ifndef _WIN32
	serial = new Serial("/dev/ttyO1");
	serial->init(B_115200_BITERATE,8,1,'N');
	DigitalMeter = new DigitalMeter_Dl64597();

#endif

}

Rs485Lister::~Rs485Lister()
{
	// TODO Auto-generated destructor stub
#ifndef _WIN32
	delete serial;
#endif
}
quint16 Rs485Lister::sendDataToComm(char* data, quint16 len)
{
	return serial->sendData(data, len);
}

static int send_counter = 0;
void Rs485Lister::sendvDump(const char* dataBuffer, int dataLen)
{
	send_counter++;

	QString filename = "dat/send/" + QString::number(send_counter);
	QFile file(filename);
	if (file.open(QIODevice::WriteOnly))
	{
		file.write(dataBuffer, dataLen);
		file.close();
	}
}

void Rs485Lister::Rs485Stop()
{
	runstate = false;
}

void Rs485Lister::run()
{
	char recvBuffer[max_recv_buffer];
	int revlen = 0;
	int len = 0;
	int time = 0;  //接收次数
	quint16 delaytime = 20;
	runstate = true;
	set_answer_delay(delaytime);
	while (runstate)
	{
#ifndef _WIN32
		//logDebug("485 send data...");
		mutex.lock();
		if (SendCmdToComm(IO_DEVICE_ADDR_KND, FN_ART_READ_KEEP_CHANNEL, 0x0200,	12))
		{
			len = 0;
			revlen = 0;
			time = 0;
			while (time++ < 10 && runstate)
			{
				len = serial->readData(recvBuffer + revlen, max_recv_buffer);
				if (len > 0)
				{
					revlen += len;
					if (revlen >= 29)
					{
						break;
					}
				}
				else
				{
					if(len == -1)
						logDebug(" 485  error -1!");
				}
				QThread::msleep(10);
			}
			if (revlen > 0)
			{
				emit serialdataReady(recvBuffer, revlen);

			}
			else
			{
				//logDebug(" 485 revice error! revlen =%d", revlen);
			}

		}
		if(DigitalMeter->getReadMeterFlag())
		{
			quint32 masterdata;
			DigitalMeter->setSerialHandle(serial);
			DigitalMeter->DigitalMeter_send_read_ele_frame();
			DigitalMeter ->DigitalMeter_read_data(&masterdata);
			DigitalMeter ->saveMasterData(masterdata);
			//DigitalMeter->setReadMeterFlag(false);
			serial->init(B_115200_BITERATE,8,1,'N');
		}
		mutex.unlock();
#endif
		QThread::msleep(100);
	}
}

quint16 Rs485Lister::SendCmdToComm(quint8 ucDevAddr, quint8 ucFunNo,
		quint16 uwStartAddr, quint16 uwData)
{
	quint8 ucBuf[1024];
	quint16 ucCnt = 0;
	quint16 uwCheckSum = 0;

	// 4字节长度的总线空闲时间
	// 地址1B
	ucBuf[ucCnt++] = ucDevAddr;
	// 功能码1B
	ucBuf[ucCnt++] = ucFunNo;
	// 起始地址	2B
	ucBuf[ucCnt++] = (quint8) (uwStartAddr >> 8);
	ucBuf[ucCnt++] = (quint8) (uwStartAddr);
	// 寄存器个数2B
	ucBuf[ucCnt++] = (quint8) (uwData >> 8);
	ucBuf[ucCnt++] = (quint8) (uwData);
	// CRC16  2B
	uwCheckSum = crc16(ucBuf, ucCnt);
	ucBuf[ucCnt++] = (quint8) (uwCheckSum);
	ucBuf[ucCnt++] = (quint8) (uwCheckSum >> 8);

	// 4字节长度的总线空闲时间
	if (sendDataToComm((char*) ucBuf, ucCnt) != ucCnt)
	{
		logDebug("sendDataToComm failed!");
		return false;
	}
	return true;
}

quint16 Rs485Lister::crc16(quint8 *pucBuf, quint16 uwLen)
{
	quint16 uwCRC = 0xffff;
	quint16 i, j, k;
	if (uwLen > 0)
	{
		for (i = 0; i < uwLen; i++)
		{
			uwCRC ^= pucBuf[i];
			for (j = 0; j < 8; j++)
			{
				if ((uwCRC & 0x0001) != 0)
				{
					k = uwCRC >> 1;
					uwCRC = k ^ 0xA001;
				}
				else
				{
					uwCRC >>= 1;
				}
			}
		}
	}

	return uwCRC;
}

bool Rs485Lister::SetTempCalibration(quint8 chinnel, qint16 value)
{
	quint8 ucBuf[MAX_MODBUS_FRAME_LEN];
	quint8 i = 0;
	quint8 ucCnt = 0;
	quint16 uwCheckSum = 0;

	// 地址1B
	ucBuf[ucCnt++] = IO_DEVICE_ADDR_KND;
	// 功能码1B
	ucBuf[ucCnt++] = FN_WRITE_REG;
	// 起始地址2B
	ucBuf[ucCnt++] = (quint8)((IO_REG_ADDR_CAB_TEMPER_0 + chinnel) >> 8);
	ucBuf[ucCnt++] = (quint8)(IO_REG_ADDR_CAB_TEMPER_0 + chinnel);
	// 寄存器个数2B
	ucBuf[ucCnt++] = (quint8)(1 >> 8);
	ucBuf[ucCnt++] = (quint8)(1);
	// 数据长度1B
	ucBuf[ucCnt++] = 2;
	// 数据,寄存器个数×2字节，每个数据高字节在前2B*RegNum
	//数据内容
	ucBuf[ucCnt++] = (quint8)(value >> 8);
	ucBuf[ucCnt++] = (quint8)(value);

	// CRC16    2B
	uwCheckSum = crc16(ucBuf, ucCnt);
	ucBuf[ucCnt++] = (quint8)(uwCheckSum);
	ucBuf[ucCnt++] = (quint8)(uwCheckSum >> 8);

	mutex.lock();
	if (sendDataToComm((char*) ucBuf, ucCnt) == ucCnt)
	{
		logDebug("sendDataToComm failed!");
		mutex.unlock();
		return true;
	}

	mutex.unlock();
	return false;
}

bool Rs485Lister::set_answer_delay(quint16 puwData)
{
	quint8 ucBuf[MAX_MODBUS_FRAME_LEN];
	//quint8 i = 0;
	quint8 ucCnt = 0;
	quint16 uwCheckSum = 0;

	// 4字节长度的总线空闲时间

	// 地址1B
	ucBuf[ucCnt++] = IO_DEVICE_ADDR_KND;
	// 功能码1B
	ucBuf[ucCnt++] = FN_WRITE_REG;
	// 起始地址2B
	ucBuf[ucCnt++] = (quint8) (0x0011 >> 8);
	ucBuf[ucCnt++] = (quint8) (0x0011);
	// 寄存器个数2B
	ucBuf[ucCnt++] = (quint8) (1 >> 8);
	ucBuf[ucCnt++] = (quint8) (1);
	// 数据长度1B
	ucBuf[ucCnt++] = 1 * 2;
	// 数据,寄存器个数×2字节，每个数据高字节在前2B*RegNum

	ucBuf[ucCnt++] = (quint8) (puwData >> 8);
	ucBuf[ucCnt++] = (quint8) (puwData);

	// CRC16    2B
	uwCheckSum = crc16(ucBuf, ucCnt);
	ucBuf[ucCnt++] = (quint8) (uwCheckSum);
	ucBuf[ucCnt++] = (quint8) (uwCheckSum >> 8);
	// 4字节长度的总线空闲时间

	/*if(uart_write(MODBUS_PORT, ucBuf, ucCnt) != ucCnt)
	 {
	 return FALSE;
	 }*/
	mutex.lock();
	if (sendDataToComm((char*) ucBuf, ucCnt) != ucCnt)
	{
		logDebug("sendDataToComm failed!");
		mutex.unlock();
		return false;
	}
	mutex.unlock();
	return true;
}
