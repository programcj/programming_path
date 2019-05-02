/*
 * OrderMainOperation.cpp
 *
 *  Created on: 2015年4月13日
 *      Author: cj
 */

#include "OrderMainOperation.h"
#include "../Public/public.h"
#include "../AppConfig/AppInfo.h"

OrderMainOperation OrderMainOperation::instance;

OrderMainOperation& OrderMainOperation::GetInstance()
{
	return instance;
}

OrderMainOperation::OrderMainOperation()
{
	mainBoyIndex = 0;
}

void OrderMainOperation::init()
{
	connect(&CSCPAction::GetInstance(), SIGNAL(OnSignalOrderTableChange()),
			this, SLOT(OnOrderTableChange()));
}

void OrderMainOperation::OnUpdate()
{
	mainOrderCache.init();
	if (Order::query(mainOrderCache, 1))
		mainOrderCache.getBoy();
	else
	{
		//否则就是没有主单的情况 生成空单
		logInfo("创键空单");
		mainOrderCache.setMainOrderFlag(1);
		mainOrderCache.subimt();
	}
	emit OnSignalUpdateMainOrder();
}

void OrderMainOperation::OrderBoyToQMap(const OrderBoy &boy, int boyIndex,
		QMap<QString, QString> &map)
{
	map.insert("派工单号", boy.getMoDispatchNo());
	map.insert("工单号", boy.getPcsMo());
	map.insert("产品编号", boy.getPcsItemNo());
	map.insert("产品描述", boy.getPcsItemName());
	map.insert("模具编号", boy.getPcsMouldNo());
	map.insert("派工数量", unit::Tool::IntToQString(boy.getPcsDispatchQty()));
	map.insert("模具可用模穴数", unit::Tool::IntToQString(boy.getPcsSocketNum1()));
	map.insert("本件模穴数", unit::Tool::IntToQString(boy.getPcsSocketNum1()));
	map.insert("产品可用模穴数", unit::Tool::IntToQString(boy.getPcsSocketNum2()));
	map.insert("使用模穴数", unit::Tool::IntToQString(boy.getPcsSocketNum2()));
	map.insert("模具适应机型吨位最小值",
			unit::Tool::IntToQString(boy.getPcsFitMachineMin()));
	map.insert("模具适应机型吨位最大值",
			unit::Tool::IntToQString(boy.getPcsFitMachineMax()));

	map.insert("最小吨位", unit::Tool::IntToQString(boy.getPcsFitMachineMin()));
	map.insert("最大吨位", unit::Tool::IntToQString(boy.getPcsFitMachineMax()));

	map.insert("次品总数", unit::Tool::IntToQString(boy.getPcsBadQty()));
    //map.insert("生产总数", 找不到模次？？？
    //		unit::Tool::IntToQString(boy.getTotalOkNum() + boy.getPcsBadQty()));

	//QList<int> PCS_BadData;		//每件产品的次品数据（对应次品原因的数据）
	map.insert("调机数", unit::Tool::IntToQString(boy.getPcsAdjNum()));
	map.insert("良品数", unit::Tool::IntToQString(boy.getTotalOkNum()));
	map.insert("本班生产总数", unit::Tool::IntToQString(boy.getCurClassProductNo()));
	map.insert("本班次品总数", unit::Tool::IntToQString(boy.getCurClassBadNo()));
	map.insert("本班良品数", unit::Tool::IntToQString(boy.getCurClassOKNum()));
	map.insert("本班打磨数", unit::Tool::IntToQString(boy.getCurClassPolishNo()));
	map.insert("本班巡机数", unit::Tool::IntToQString(boy.getCurClassInspectNo()));
	{
		QString str;
		str.sprintf("%.1f", boy.getCurClassBadRate());
		str += "%";
		map.insert("本班次品率", str); //
	}
	map.insert("调机次品数", unit::Tool::IntToQString(boy.getAdjDefCount()));
	map.insert("调机良品数", unit::Tool::IntToQString(boy.getADJOKNum()));
	map.insert("调机空模数", unit::Tool::IntToQString(boy.getADJEmptyMoldNum()));
	map.insert("当前件", unit::Tool::IntToQString(boyIndex + 1));
}

void OrderMainOperation::OrderToQMap(const Order &order,
		QMap<QString, QString> &map, int subIndex,
		QMap<QString, QString> &subMap)
{
	QString str;
	map.insert("派工单号", order.getMoDispatchNo());
	map.insert("派工项次", order.getMoDispatchPrior());
	map.insert("工序代码", order.getMoProcCode());
	map.insert("工序名称", order.getMoProcName());
	map.insert("站别代码", order.getMoStaCode());
	map.insert("站别名称", order.getMoStaName());
	map.insert("工作中心", order.getMoStaName());
	str.sprintf("%d S", order.getMoStandCycle() / 1000);
	map.insert("标准周期", str);

    str.sprintf("%d", order.getMoTotalNum());
	map.insert("模次", str);

	str.sprintf("%d", order.getMoMultiNum());
	map.insert("总件数", str);

	str.sprintf("%d", order.getMoBadTypeNo());
	map.insert("次品类型选项", str);

	str.sprintf("%d", order.getMoBadReasonNum());
	map.insert("次品原因总数", str);

	str.sprintf("%d", order.getMainOrderFlag());
	map.insert("主单序号", str);

    str.sprintf("%d", order.getMoTotalNum());
	map.insert("模次", str);

	if (subIndex >= 0 && subIndex < order.orderBoyList.size())
	{
		OrderBoyToQMap(order.orderBoyList[subIndex], subIndex, subMap);
		str.sprintf("%d / %d", order.getMoMultiNum(), subIndex + 1);
        map.insert("总件/当前", str);

        subMap.insert("生产总数",
                      unit::Tool::IntToQString(order.getMoTotalNum()*order.orderBoyList[subIndex].getPcsSocketNum2()));
	}
}

//更新 哪一件的No1 调机良品数
void OrderMainOperation::OnAddADJOKNum(int subIndex, int value)
{
	OrderBoy boy = mainOrderCache.orderBoyList[subIndex];
    boy.OnAddADJOKNum(value);
}

void OrderMainOperation::OnAddADJEmptyMoldNum(int subIndex, int value)
{
	OrderBoy boy = mainOrderCache.orderBoyList[subIndex];
    boy.OnAddADJEmptyMoldNum(value);
}

void OrderMainOperation::OnUpdateADJDefList(int subIndex, const QString &str)
{
	OrderBoy boy = mainOrderCache.orderBoyList[subIndex];
    boy.OnUpdateADJDefList(str);
}

// 换班的操作
void OrderMainOperation::OnReplaceClass()
{
	for (int i = 0; i < mainOrderCache.orderBoyList.size(); i++)
	{
		mainOrderCache.orderBoyList[i].OnReplaceClass();
	}
	OnUpdate();
}

//换班处理
bool OrderMainOperation::OnReplaceClassProcess()
{
	QDateTime dateTime = QDateTime::currentDateTime();
	QString classTime = dateTime.toString("yyyy-MM-dd hh:mm");
	//换班条件 当前时间在     换班时间表中, 且与上次换班时间不等
	//换班之后 重设上次换班时间
	for (int i = 0; i < AppInfo::cfg_class.ClassTimeList.size(); i++)
	{
		if (dateTime.time().hour() == AppInfo::cfg_class.ClassTimeList[i].wHour
				&& dateTime.time().minute()
						== AppInfo::cfg_class.ClassTimeList[i].wMinute)
		{
			if (classTime != AppInfo::cfg_class.ChangeTime)
			{
				//换班了哦
				OrderMainOperation::GetInstance().OnReplaceClass();
				AppInfo::cfg_class.ChangeTime = classTime;
				AppInfo::cfg_class.save();
				logInfo(QString("换班:%1").arg(classTime));
			}
			return true;
		}
	}
	return false;
}

//自动换单
void OrderMainOperation::OnReplaceOrder()
{
	mainOrderCache.remove();
	Order::sort(); //重新排序
	OnUpdate();

	BrushCard brush(mainOrderCache, "00000001", BrushCard::AsCHANGE_ORDER,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook("自动换单开始", brush).insert();
	brush.setIsBeginEnd(1);
	Notebook("自动换单结束", brush).insert();
}

void OrderMainOperation::OnAddModeCount(const QString &endCycle, //生产数据产生时间
		QList<int> &temperList,	//N路温度 当前为8路 固定
		unsigned long MachineCycle, //机器周期，毫秒
		unsigned long FillTime, //填充时间，毫秒
		unsigned long CycleTime, //成型周期
		QList<int> &keepList,	//100个压力点
		const QString &startCycle, //生产数据开始时间
		unsigned long TotalNum) //模次 可以不用填写这个
{
	if (AppInfo::GetInstance().getHaveStopCard()) //有停机卡就不记模
		return;

    mainOrderCache.OnAddMOTotalNum(1);
	//每一件的显示呢? 也须要增加
	for (int i = 0; i < mainOrderCache.orderBoyList.size(); i++)
	{

		mainOrderCache.orderBoyList[i].OnAddCurClassProductNo(
				1 * mainOrderCache.orderBoyList[i].getPcsSocketNum2());
	}

	Producted producted;
	producted.setDispatchNo(mainOrderCache.getMoDispatchNo()); //	20	ASC	派工单号
	producted.setDispatchPrior(mainOrderCache.getMoDispatchPrior()); // 30	ASC	派工项次
	producted.setProcCode(mainOrderCache.getMoProcCode()); //	20	ASC	工序代码
	producted.setStaCode(mainOrderCache.getMoStaCode()); //	10	ASC	站别代码

	producted.setEndCycle(endCycle); //	6	HEX	生产数据产生时间（成型周期结束时间）
	producted.getTemperValue().append(temperList); //setTemperValue	2*N	HEX	N路温度值，每路2字节（依序为温度1、温度2、……、温度N）
	producted.setMachineCycle(MachineCycle); //	4	HEX	机器周期，毫秒
	producted.setFillTime(FillTime); //		4	HEX	填充时间，毫秒
	producted.setCycleTime(CycleTime); //		4	HEX	成型周期，毫秒
    producted.setTotalNum(mainOrderCache.getMoTotalNum()); //		4	HEX	模次（成模次序，原啤数）
	producted.getKeepPress().append(keepList); //	2*100	HEX	100个压力点
	producted.setStartCycle(startCycle); //	6	HEX	生产数据开始时间（成型周期开始时间）
	producted.subimt();

    emit OnSignalModeNumberAdd(mainOrderCache.getMoTotalNum(), startCycle,
			endCycle, MachineCycle, FillTime, CycleTime);

	{ //判断 达到生产数是否自动换单
		if (AppInfo::GetInstance().sys_func_cfg.isAutoChangeOrder())
		{
			if (mainOrderCache.orderBoyList.size() > 0
					&& mainOrderCache.orderBoyList[0].getTotalOkNum()
							>= mainOrderCache.orderBoyList[0].getPcsDispatchQty())
			{
				//比较生产数
				OnReplaceOrder(); //自动换单
			}
		}
	}
	OnUpdate();
}

/**
 * 把原料信息转换成Map列表 中文名+值
 */
void OrderMainOperation::MaterialToMap(const Material &material,
		QMap<QString, QString>& map)
{
	map.insert("派工单号", material.getDispatchNo());
	map.insert("原料编号", material.getMaterialNo());
	map.insert("原料名称", material.getMaterialName());
    map.insert("原料批次", material.getBatchNo());
    map.insert("投料数量(Kg)", material.getFeedingQty());
    map.insert("投料时间", material.getFeedingTime());
}

//当网络通信的业务有工单插入时
void OrderMainOperation::OnOrderTableChange()
{
//if (Order::count() == 1)
	OnUpdate();
}

bool OrderMainOperation::OnUpdateStandCycle(int cycle)
{
	logInfo(QString("修改周期 %1:%2").arg(mainOrderCache.getMoDispatchNo()).arg(	cycle));
	bool ret = mainOrderCache.OnUpdateStandCycle(cycle);
	OnUpdate();
	return ret;
}

bool OrderMainOperation::OnDelete()
{
	Order order;
	Order::query(order, 1);
	order.remove();
	Order::sort();
	OnUpdate();
    return true;
}
