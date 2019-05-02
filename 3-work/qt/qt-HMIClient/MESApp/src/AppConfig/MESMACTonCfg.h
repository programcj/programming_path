/*
 * MESMachineTon.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESMACHINETON_H_
#define MESMACHINETON_H_

#include <QtCore>
#include <QString>
#include <QList>
/*
 *机器吨位配置文件
 //	版本号：6字节的生成日期(HEX)
 //	机器总数：2字节（n）
 //	第1机器：
 //	序号：2字节
 //	机器编号：20字节
 //	吨位值：4字节
 //	第2机器：
 //	序号：2字节	功能名称：30字节
 //	机器编号：20字节	功能序号：2字节
 //	吨位值：4字节	功能值：2字节
 //
 //	……
 //	第n机器：
 //	序号：2字节	功能名称：30字节
 //	机器编号：20字节	功能序号：2字节
 //	吨位值：4字节	功能值：2字节
 //
 */
class MESMACTonCfg
{
public:
	class TonInfo
	{
		int index; //	序号：2字节
		QString machine; //	机器编号：20字节
		int ton; //	吨位值：4字节
		QString ipaddress;	//机器IP
		QString machinoBrand; //品牌

	public:
		TonInfo()
		{
			index = 0;
			ton = 0;
		}
		TonInfo(int index, const char *machine,
				const char *ipaddr,
				const char *machinoBrand,
				int ton)
		{
			this->index = index;
			this->machine = QString(machine);
			this->ton = ton;
			this->ipaddress =QString(ipaddr);
			this->machinoBrand = QString(machinoBrand);
		}

		int getIndex() const
		{
			return index;
		}

		void setIndex(int index)
		{
			this->index = index;
		}

		const QString& getMachine() const
		{
			return machine;
		}

		void setMachine(const QString& machine)
		{
			this->machine = machine;
		}

		int getTon() const
		{
			return ton;
		}

		void setTon(int ton)
		{
			this->ton = ton;
		}

		const QString& getIpaddress() const
		{
			return ipaddress;
		}

		void setIpaddress(const QString& ipaddress)
		{
			this->ipaddress = ipaddress;
		}

		const QString& getMachinoBrand() const
		{
			return machinoBrand;
		}

		void setMachinoBrand(const QString& machinoBrand)
		{
			this->machinoBrand = machinoBrand;
		}
	};

	char version[6]; //版本号：6字节的生成日期(HEX)
	int macSize; //机器总数：2字节（n）
	QList<TonInfo> TonList; //机器吨位列表

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESMACHINETON_H_ */
