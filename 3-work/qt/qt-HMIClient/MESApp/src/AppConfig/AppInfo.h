/*
 * AppInfo.h
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#ifndef APPINFO_H_
#define APPINFO_H_

#include "../Sqlite/Entity/absentity.h"
#include "MESBadCfg.h"
#include "MESPDFuncCfg.h"
#include "MESMACTonCfg.h"
#include "MESSysFuncCfg.h"
#include "MESFaultTypeCfg.h"
#include "MESCheckItemCfg.h"
#include "MESRepairResultCfg.h"
#include "MESCauseBadCfg.h"
#include "MESDevCfg.h"
#include "MESClassCfg.h"

using namespace entity;

/*
 * 应用配置信息
 */
class AppInfo: public AbsPropertyBase
{
	//<XML>
	QString version; //系统版本
	QString devID; //设备ID
	QString machineID; //机器ID

	QString serverIP; //服务器IP
	int serverPortA; //服务器端口号A
	int serverPortB; //服务器端口号B
	QString devIP;	//设备IP

	QString netDeviceName; //当前所用网卡名
	QString brushCardPassword; //刷卡密码
	QString BrushCardID; //刷卡人卡号

	int backLightTime; //背光时间
	int tbProductedBakCount; //生产备份表记录的大小

	int haveStopCard;	//是否有停机卡  0 没有， 1 有
	int ButtonBuzzer;	//是否按钮声音
	int UsbPower;       //是否打开USB电源
	//</XML>


public:
	AppInfo();
	//////////////////////////////////////////////////////////////////
    void init(); //初始化
	void saveConfig();
	void initNetCSCP(); //初始化网络接口
	void OnUpdateNet(const QString &ip, int portA, int portB); //理新到新的网络服务器

	static QStringList getConfigNameList(); //获取配置文件名列表


	/**
	 * 应用信息
	 */
	static const QString getPathFile_appInfo()
	{
		return QCoreApplication::applicationDirPath() + "/appInfo.xml";
	}

    //获取功能按钮状态文件
    static const QString getPathFile_FunButtonStatus()
	{
		return QCoreApplication::applicationDirPath() + "/ui_btstatus.xml";
	}

	//下载缓存目录
	static const QString getPath_DownCache()
	{
		QString path = QCoreApplication::applicationDirPath() + "/caches/";
		if (!QDir().exists(path))
		{
			QDir().mkdir(path);
		}
		return path;
	}

	//临时文件目录
	static const QString getPath_Tmp()
	{
		QString path = QCoreApplication::applicationDirPath() + "/tmp/";
		if (!QDir().exists(path))
		{
			QDir().mkdir(path);
		}
		return path;
	}

	//配置文件目录
	static const QString getPath_Config()
	{
		QString path = QCoreApplication::applicationDirPath() + "/config_file/";
		if (!QDir().exists(path))
		{
			QDir().mkdir(path);
		}
		return path;
	}

	//日志文件目录
	static const QString getPath_Logs()
	{
		QString path = QCoreApplication::applicationDirPath() + "/logs/";
		if (!QDir().exists(path))
		{
			QDir().mkdir(path);
		}
		return path;
	}

	//数据库目录
	static const QString getPath_Database()
	{
		QString path = QCoreApplication::applicationDirPath() + "/databases/";
		if (!QDir().exists(path))
		{
			QDir().mkdir(path);
		}
		return path;
	}

	//60、	次品原因配置文件：bad_cfg.bin
	static MESBadCfg bad_cfg;
	//61、	管理者信息配置文件：mng_info_cfg.bin
	//62、	生产工单功能区按键配置文件：pd_func_cfg.bin
	static MESPDFuncCfg pd_func_cfg;
	//63、	设备配置文件：dev_cfg.bin
	static MESDevCfg dev_cfg;
	//64、	系统功能配置文件：sys_func_cfg.bin
	static MESSysFuncCfg sys_func_cfg;
	//65、	机器吨位配置文件：mac_ton_cfg.bin
	static MESMACTonCfg mac_ton_cfg;
	//66、	故障类型文件：fault_type_cfg.bin
	static MESFaultTypeCfg fault_type_cfg;
	//67、	点检项目文件：check_item_cfg.bin
	static MESCheckItemCfg check_item_cfg;
	//68、	维修结果配置文件：repair_result_cfg.bin
	static MESRepairResultCfg repair_result_cfg;
	//69、	次品产生原因配置文件：cause_bad_cfg.bin
	static MESCauseBadCfg cause_bad_cfg;

	//班次配置信息
	static MESClassCfg cfg_class;

	static AppInfo instance;
	static AppInfo &GetInstance();

	////////////////////////////////////////////////////////////////////////
	// 以下为数据的get set等操作
	////////////////////////////////////////////////////////////////////////
	virtual const char *getClassName() const
	{
		return GetThisClassName();
	}

	static const char *GetThisClassName()
	{
		return "AppInfo";
	}

	virtual void onBindProperty();

	const QString& getBrushCardPassword() const
	{
		return brushCardPassword;
	}

	void setBrushCardPassword(const QString& brushCardPassword)
	{
		this->brushCardPassword = brushCardPassword;
	}

	const QString& getDevId() const
	{
		return devID;
	}

	void setDevId(const QString& devId)
	{
		devID = devId;
	}

	const QString& getMachineId() const
	{
		return machineID;
	}

	void setMachineId(const QString& machineId)
	{
		machineID = machineId;
	}

	const QString& getServerIp() const
	{
		return serverIP;
	}

	void setServerIp(const QString& serverIp)
	{
		serverIP = serverIp;
	}

	const int& getServerPortA() const
	{
		return serverPortA;
	}

	void setServerPortA(const int& serverPortA)
	{
		this->serverPortA = serverPortA;
	}

	const int& getServerPortB() const
	{
		return serverPortB;
	}

	void setServerPortB(const int& serverPortB)
	{
		this->serverPortB = serverPortB;
	}

	const QString& getVersion() const
	{
		return version;
	}

	void setVersion(const QString& version)
	{
		this->version = version;
	}

	const QString& getBrushCardId() const
	{
		return BrushCardID;
	}

	void setBrushCardId(const QString& brushCardId)
	{
		BrushCardID = brushCardId;
	}
	int getTbProductedBakCount() const
	{
		return tbProductedBakCount;
	}

	void setTbProductedBakCount(int tbProductedBakCount)
	{
		this->tbProductedBakCount = tbProductedBakCount;
	}

	//停机卡是否有
	bool getHaveStopCard() const
	{
		return haveStopCard == 1;
	}

	//设定停机卡
	void setHaveStopCard(bool flag)
	{
		if (flag)
			this->haveStopCard = 1;
		else
			haveStopCard = 0;
	}

	//按钮声音
	bool isButtonBuzzer() const
	{
		return ButtonBuzzer == 1;
	}

	//按钮声音
	void setButtonBuzzer(bool flag)
	{
		if (flag)
			ButtonBuzzer = 1;
		else
			ButtonBuzzer = 0;
	}

	//usb电源
	bool isUsbPower() const
	{
		return UsbPower == 1;
	}

	//usb电源
	void setUsbPower(bool flag)
	{
		if (flag)
			UsbPower = 1;
		else
			UsbPower = 0;
	}
	//设备IP
	const QString& getDevIp() const
	{
		return devIP;
	}
	//设备IP
	void setDevIp(const QString& devIp)
	{
		devIP = devIp;
	}
	//网络设备名
	const QString& getNetDeviceName() const
	{
		return netDeviceName;
	}
	//网络设备名
	void setNetDeviceName(const QString& netDeviceName)
	{
		this->netDeviceName = netDeviceName;
	}
	//背光时间
	int getBackLightTime() const
	{
		return backLightTime;
	}
	//背光时间
	void setBackLightTime(int backLightTime)
	{
		this->backLightTime = backLightTime;
	}
};

#endif /* APPINFO_H_ */
