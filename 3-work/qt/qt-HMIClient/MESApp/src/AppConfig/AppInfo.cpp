/*
 * AppInfo.cpp
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#include <string.h>
#include "AppInfo.h"
#include "QFile"
#include "../Sqlite/Entity/Employee.h"
#include "../Public/public.h"

//60、	次品原因配置文件：bad_cfg.bin
MESBadCfg AppInfo::bad_cfg;
//61、	管理者信息配置文件：mng_info_cfg.bin
//62、	生产工单功能区按键配置文件：pd_func_cfg.bin
MESPDFuncCfg AppInfo::pd_func_cfg;
//63、	设备配置文件：dev_cfg.bin
MESDevCfg AppInfo::dev_cfg;
//64、	系统功能配置文件：sys_func_cfg.bin
MESSysFuncCfg AppInfo::sys_func_cfg;
//65、	机器吨位配置文件：mac_ton_cfg.bin
MESMACTonCfg AppInfo::mac_ton_cfg;
//66、	故障类型文件：fault_type_cfg.bin
MESFaultTypeCfg AppInfo::fault_type_cfg;
//67、	点检项目文件：check_item_cfg.bin
MESCheckItemCfg AppInfo::check_item_cfg;
//68、	维修结果配置文件：repair_result_cfg.bin
MESRepairResultCfg AppInfo::repair_result_cfg;
//69、	次品产生原因配置文件：cause_bad_cfg.bin
MESCauseBadCfg AppInfo::cause_bad_cfg;
//班次配置信息
MESClassCfg AppInfo::cfg_class;

AppInfo AppInfo::instance;


//获取config_file目录下文件列表
QStringList AppInfo::getConfigNameList()
{
	QStringList NameList;
	NameList.append("bad_cfg.bin"); //次品名
	NameList.append("mng_info_cfg.bin"); //管理者信息配置文件
	NameList.append("pd_func_cfg.bin"); //生产工单功能区按键配置文件
	NameList.append("dev_cfg.bin");     //设备配置文件
	NameList.append("sys_func_cfg.bin");//系统功能配置文件
	NameList.append("mac_ton_cfg.bin"); //机器吨位配置文件
	NameList.append("fault_type_cfg.bin"); //故障类型文件
	NameList.append("check_item_cfg.bin"); //点检项目文件
	NameList.append("repair_result_cfg.bin"); //维修结果配置文件
	NameList.append("cause_bad_cfg.bin");    //次品产生原因配置文件

	return NameList;
}

//b)	“管理者信息配置文件”文件格式	mng_info_cfg.bin
static void Init_mng_info_cfg_bin()
{
	char version[6];	//	版本号：6字节的生成日期(HEX)
	quint16 sumSize = 0;	//	管理员总数：2字节（n）

    Employee user;
    user.setIDCardNo("00000001");
    user.setIcCardRight("FFFFFFFF");
    user.setEmpID("00000001");
    user.setEmpNameCN("系统管理员");
    user.setPost("系统管理员");
    user.subimt();

	QFile fileRead(AppInfo::getPath_Config() + "mng_info_cfg.bin");
	if (fileRead.open(QIODevice::ReadOnly))
	{
		QDataStream readDataStream(&fileRead);
		readDataStream.readRawData(version, sizeof(version));
		readDataStream.readRawData((char*) &sumSize, 2);
		QString versionStr;
		versionStr.sprintf("%02d%02d%02d%02d%02d%02d", version[0], version[1],
				version[2], version[3], version[4], version[5]);

        if( sqlite::SQLiteBaseHelper::getInstance().versionCheck("mng_info_cfg.bin",versionStr) )
        {
            fileRead.close();
            return ;
        }
        sqlite::SQLiteBaseHelper::getInstance().versionSave("mng_info_cfg.bin",versionStr);

		for (int i = 0; i < sumSize; i++)
		{
			quint8 status;
			char card[10];
			char q[16];
			char workNo[20];
			char name[12];
			char point[20];
			int picPoint;
			int picLen;
			readDataStream >> status;	//	状态：1字节    注释：1：新增，2：修改，3：删除
			readDataStream.readRawData(card, sizeof(card));	//	卡号:   10字节
			readDataStream.readRawData(q, sizeof(q));	//	卡权限:   16字节
			readDataStream.readRawData(workNo, sizeof(workNo));	//	工号：20字节
			readDataStream.readRawData(name, sizeof(name));	//	姓名：12字节
			readDataStream.readRawData(point, sizeof(point));	//	职位：20字节
			readDataStream >> picPoint;	//	图片起始地址：4字节
			readDataStream >> picLen;	//	图片长度： 4字节
			Employee user;
			user.setIDCardNo(card);
			user.setIcCardRight(q);
			user.setEmpID(workNo);
			user.setEmpNameCN(name);
			user.setPost(point);

			switch (status)
			{
			case 1: //新增
			case 2: //修改
				user.subimt();
				break;
			case 3: //删除
				user.remove();
				break;
			}
		}
		fileRead.close();
	}
}

void AppInfo::init()
{
	//
	QString AppInfoPath = getPathFile_appInfo();
	QFile file(AppInfoPath);
	if (!QFile::exists(AppInfoPath))
	{
		AppInfo info;
		info.setServerIp("192.168.8.223");
		info.setServerPortA(4007);
		info.setServerPortB(4008);
		info.setTbProductedBakCount(2000);

		QString str = PropertyBaseToXML(info).toString();
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream out(&file);
			out << str;
			file.close();
		}
	}
	if (file.open(QIODevice::ReadOnly | QFile::Text))
	{
		XMLFileToPropertyBase(file, instance);
		file.close();
	}

	if (getVersion() != APP_VER_STR)
	{
		setVersion(APP_VER_STR);
		saveConfig();
	}

    //初始化配置文件
	Init_mng_info_cfg_bin();

    //次品原因配置文件 bad_cfg.bin
     bad_cfg.read(AppInfo::getPath_Config() + "bad_cfg.bin");

    //生产工单功能区按键配置文件”文件格式	pd_func_cfg.bin
    pd_func_cfg.read(AppInfo::getPath_Config() + "pd_func_cfg.bin");

    //a)	“设备配置文件”文件格式	dev_cfg.bin
    dev_cfg.read(AppInfo::getPath_Config() + "dev_cfg.bin");

    //b)	“系统功能配置文件”文件格式sys_func_cfg.bin
    sys_func_cfg.read(AppInfo::getPath_Config() + "sys_func_cfg.bin");

    //机器吨位配置文件”文件格式mac_ton_cfg.bin
    mac_ton_cfg.read(AppInfo::getPath_Config() + "mac_ton_cfg.bin");

    //故障类型配置文件”文件格式fault_type_cfg.bin
    fault_type_cfg.read(AppInfo::getPath_Config() + "fault_type_cfg.bin");

    //e)	“点检项目配置文件”文件格式check_item_cfg.bin
    check_item_cfg.read(AppInfo::getPath_Config() + "check_item_cfg.bin");

    //维修结果配置文件”文件格式repair_result_cfg.bin
    repair_result_cfg.read(AppInfo::getPath_Config() + "repair_result_cfg.bin");

    cause_bad_cfg.read(AppInfo::getPath_Config() + "cause_bad_cfg.bin");

	cfg_class.init(); //班次初始化
}

AppInfo &AppInfo::GetInstance()
{
	return instance;
}

AppInfo::AppInfo()
{
	version = "";
	serverPortA = 0;
	serverPortB = 0;
    BrushCardID = "00000001";
    tbProductedBakCount = 1000;
	UsbPower = 1;
	backLightTime = 30;
    haveStopCard=0;
    ButtonBuzzer=1;
}

void AppInfo::onBindProperty()
{
	addProperty("version", Property::AsQStr, &version); //系统版本
	addProperty("devID", Property::AsQStr, &devID); //设备ID
	addProperty("machineID", Property::AsQStr, &machineID); //机器ID
	addProperty("serverIP", Property::AsQStr, &serverIP); //服务器IP
	addProperty("serverPortA", Property::AsInt, &serverPortA); //服务器端口号A
	addProperty("serverPortB", Property::AsInt, &serverPortB); //服务器端口号B
	addProperty("brushCardPassword", Property::AsQStr, &brushCardPassword);
	addProperty("BrushCardID", Property::AsQStr, &BrushCardID);	//当前刷卡人,请在刷卡界面更新
	addProperty("devIP", Property::AsQStr, &devIP);	//设备IP
	addProperty("netDeviceName", Property::AsQStr, &netDeviceName);	//网络设备名
	addProperty("tbProductedBakCount", Property::AsInt, &tbProductedBakCount); //生产备份表记录的大小
	addProperty("haveStopCard", Property::AsInt, &haveStopCard); //是否有停机卡
	addProperty("ButtonBuzzer", Property::AsInt, &ButtonBuzzer); //是否有按键声
	addProperty("UsbPower", Property::AsInt, &UsbPower); //是否打开usb设备

	addProperty("backLightTime", Property::AsInt, &backLightTime); //背光时间
}

void AppInfo::saveConfig()
{
	QFile file(getPathFile_appInfo());

	QString str = PropertyBaseToXML(AppInfo::GetInstance()).toString();
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << str;
		file.close();
	}
}

void AppInfo::initNetCSCP()
{
	//启动网络数据交互
	CSCPNotebookTask::GetInstance().startCSCPServer(
			AppInfo::GetInstance().getServerIp(),
			AppInfo::GetInstance().getServerPortA(),
			AppInfo::GetInstance().getServerPortB());
}
//更新网络连接信息
void AppInfo::OnUpdateNet(const QString& ip, int portA, int portB)
{
	setServerIp(ip);
	setServerPortA(portA);
	setServerPortB(portB);
	saveConfig();
	CSCPNotebookTask::GetInstance().startCSCPServer(
			AppInfo::GetInstance().getServerIp(),
			AppInfo::GetInstance().getServerPortA(),
			AppInfo::GetInstance().getServerPortB());
}
