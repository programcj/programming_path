/*
 * OrderMainOperation.h
 *
 *  Created on: 2015年4月13日
 *      Author: cj
 */

#ifndef ORDERMAINOPERATION_H_
#define ORDERMAINOPERATION_H_

#include "Entity/Material.h"
#include "Entity/Notebook.h"
#include "Entity/Order.h"
#include "../Unit/Tool.h"
#include <QObject>

using namespace entity;

//工单索引
class OrderIndex
{
public:
	int Index;
	int BoyIndex;
	int BoySum;
	OrderIndex()
	{
		Index = 1; //默认为主单
		BoyIndex = 0;
		BoySum = 0;
	}
};

/*
 * 主单的操作类,专用
 */
class OrderMainOperation: public QObject
{
Q_OBJECT
	static OrderMainOperation instance;
public:
	static OrderMainOperation &GetInstance();
	Order mainOrderCache;
	int mainBoyIndex;

	void init();

	OrderMainOperation();
	/**
	 * 更新数据
	 */
	void OnUpdate();

	/**
	 * 把工单信息转换成Map列表 中文名+值
	 */
	static void OrderBoyToQMap(const OrderBoy &boy, int boyIndex,
			QMap<QString, QString> &map);

	static void OrderToQMap(const Order &order, QMap<QString, QString> &map,
			int subIndex, QMap<QString, QString> &subMap);
	/**
	 * 把原料信息转换成Map列表 中文名+值
	 */
	static void MaterialToMap(const Material &material,
			QMap<QString, QString> &map);

	//修改周期
	bool OnUpdateStandCycle(int cycle);

	//更新 哪一件的No1 调机良品数
    void OnAddADJOKNum(int subIndex, int value);
    void OnAddADJEmptyMoldNum(int subIndex, int value);
    void OnUpdateADJDefList(int subIndex, const QString &str);

	bool OnReplaceClassProcess(); //换班判断
	void OnReplaceClass(); // 换班的操作
	void OnReplaceOrder(); //系统自动换单
//增加模次统计
//	PD_EndCycle	6	HEX	生产数据产生时间（成型周期结束时间）
//	PD_TemperValue	2*N	HEX	N路温度值，每路2字节（依序为温度1、温度2、……、温度N）
//	PD_MachineCycle	4	HEX	机器周期，毫秒
//	PD_FillTime	4	HEX	填充时间，毫秒
//	PD_CycleTime	4	HEX	成型周期，毫秒
//	PD_TotalNum	4	HEX	模次（成模次序，原啤数）
//	PD_KeepPress	2*100	HEX	100个压力点
//	PD_StartCycle	6	HEX	生产数据开始时间（成型周期开始时间）

	void OnAddModeCount(const QString &endCycle, //生产数据产生时间
			QList<int> &temperList,	//N路温度 当前为8路 固定
			unsigned long MachineCycle, //机器周期，毫秒
			unsigned long FillTime, //填充时间，毫秒
			unsigned long CycleTime, //成型周期
			QList<int> &keepList,	//100个压力点
			const QString &startCycle, //生产数据开始时间
			unsigned long TotalNum = 0); //模次 可以不用填写这个
	//删除主单
	bool OnDelete();

signals:
	//主单更新的信号
	void OnSignalUpdateMainOrder();
	//模次信息增加
	void OnSignalModeNumberAdd(int number, //次数
			const QString start, //开始时间
			const QString endTime, //结束时间
			unsigned long machineCycle, //机器周期，毫秒
			unsigned long FillTime, //填充时间，毫秒
			unsigned long CycleTime //成型周期
			);
private slots:
	void OnOrderTableChange();
};

#endif /* ORDERMAINOPERATION_H_ */
