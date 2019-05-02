/*
 * CSCPAction.cpp
 *
 *  Created on: 2015年3月26日
 *      Author: cj
 */

#include "CSCPAction.h"
#include "../Sqlite/OrderMainOperation.h"

CSCPAction CSCPAction::instance;

CSCPAction& CSCPAction::GetInstance()
{
	return CSCPAction::instance;
}

/**
 * 派单
 */
bool CSCPAction::OnOrderSend(Order& order,
		QList<entity::Material> &MaterialList)
{
	//如果有空的 派工单号 就删除,空单
	{
		Order torder;
		torder.remove();
	}
	if (Order::countMODispatchNo(order.getMoDispatchNo()) != 0) //此工单己存在了
		return false;
    logInfo(QString("服务器派单:%1,模:%2").arg(order.getMoDispatchNo()).arg(order.getMoTotalNum()));
	order.setMainOrderFlag(Order::count() + 1);
	if (order.subimt())
	{
		for (int i = 0; i < MaterialList.size(); i++)
		{
			MaterialList[i].subimt();
		}
		emit OnSignalOrderTableChange();
		return true;
	}
	return false;
}

/**
 * 删除工单
 */
bool CSCPAction::OnOrderDelete(const QString &no)
{
	Order order;
	QString DispatchNo;
	DispatchNo = no;
	logInfo(QString("删除派工单 %1").arg(no));
	if (!Order::query(order, DispatchNo)) //不存在的单
		return true;

    if (order.getMoTotalNum() > 0) //模次>0的单
		return false;

	if (order.getMainOrderFlag() == 1)
	{
		//如果是主单,这里创建一个换单的操作，刷卡换单
		Order orderNext;
		Order::query(orderNext, 2);
		BrushCard brush = BrushCard(orderNext, "00000001",
				BrushCard::AsCHANGE_ORDER, Tool::GetCurrentDateTimeStr(), 0);
		Notebook("23  换单 开始", brush).insert();

		brush.setIsBeginEnd(1);
		Notebook("23  换单 结束", brush).insert();

		//当然还有换班
	}
	order.remove();
	//重新排序
	Order::sort();
	emit OnSignalOrderTableChange();
	return true;
}
/**
 * 设置系统时间
 */
bool CSCPAction::OnSetSystemTime(const QDateTime &dateTime)
{
	logInfo(QString("修改系统时间:%1").arg(Tool::DateTimeToQString(dateTime)));
	QString str;
	str = "date -s \"";
	str += Tool::DateTimeToQString(dateTime);
	str += "\"";
	QProcess::execute(str);
	return true;
}
/**
 * 调整派工单
 *
 */
bool CSCPAction::OnOrderModify(int index, const QString& DispatchNo,
		const QString& DispatchPrior)
{
	Order::UpdateMainOrderFlag(DispatchNo, index); //派工项次须不须要? 我觉得不须要,因为派工单号是唯一的
	emit OnSignalOrderTableChange();
	return true;
}
/**
 * 设备换单
 */
bool CSCPAction::OnOrderReplace()
{
	OrderMainOperation order;
	order.OnUpdate();
	order.OnReplaceOrder();

	emit OnSignalOrderTableChange();
	return true;
}
/**
 * SQLite语句下发
 */
bool CSCPAction::OnSQL(const QString &sql)
{

	return true;
}
/**
 * 设置密码
 */
bool CSCPAction::OnSetDevPassword(const QString& pass)
{
	AppInfo::GetInstance().setBrushCardPassword(pass);
	AppInfo::GetInstance().saveConfig();
	return true;
}

/**
 * 配置文件初始化
 */
bool CSCPAction::OnConfigFileInit(const QString& fileName)
{

	return true;
}

/**
 * 设置班次信息
 */
bool CSCPAction::OnSetClassInfo(QList<MESControlRequest::ClassInfo> &info)
{
	AppInfo::cfg_class.ClassTimeList.clear();
	QList<ClassTime> ClassTimeList;

	for (int i = 0; i < info.count(); i++)
	{
		ClassTime time;
		time.wHour = info[i].FDAT_BeginTime[0] & 0x00FF;
		time.wMinute = (info[i].FDAT_BeginTime[i] & 0xFF00) >> 8;
		ClassTimeList.append(time);

		time.wHour = info[i].FDAT_EndTime[i] & 0x00FF;
		time.wMinute = (info[i].FDAT_EndTime[i] & 0xFF00) >> 8;
		ClassTimeList.append(time);
	}
	AppInfo::cfg_class.ClassTimeList.append(ClassTimeList);
	AppInfo::cfg_class.save();
	return true;
}

void CSCPAction::OnCSCPConnect(bool flag, const QString& mess)
{
	emit OnSignalCSCPConnect(flag, mess);
}

void CSCPAction::OnConfigDownSucess(const QString& name)
{
	AppInfo::GetInstance().init();
	emit OnSignalConfigRefurbish(name);
}

void CSCPAction::OnMailMess(QString startT, QString endT, QString mess)
{
	emit OnSignalMESMailMess(startT, endT, mess);
}
