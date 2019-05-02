/*
 * SQLiteBaseHelper.cpp
 *
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#include "SQLiteBaseHelper.h"
#include "../Unit/MESLog.h"

namespace sqlite
{
QString tb_order = "CREATE TABLE IF NOT EXISTS tb_order ("
		"MO_DispatchNo varchar(30) PRIMARY KEY,"	//派工单号
		"MO_DispatchPrior varchar(30),"//派工项次
		"MO_ProcCode varchar(30),"//工序代码
		"MO_ProcName varchar(55),"//工序名称
		"MO_StaCode varchar(30),"//站别代码
		"MO_StaName varchar(30),"//站别名称（工作中心）
		"MO_StandCycle integer,"//标准周期，毫秒
        "MO_TotalNum integer,"//模次,生产次数（原啤数） 本班产品数　本班次品数
		"MO_MultiNum integer,"//总件数N（0< N <= 100）
		"MO_BadTypeNo integer,"//次品类型选项（配置文件中）
		"MO_BadReasonNum integer,"//次品原因总数M（0< M <= 100）
        "MainOrder_FLAG integer,"//主单序号
		"No1 integer,No2 integer,"
		"Str1 varchar,Str2 varchar"//预留的
		")";

QString tb_orderBoy = "CREATE TABLE IF NOT EXISTS tb_orderBoy (" //总件数
				"	ID integer PRIMARY KEY autoincrement,"
				"	MO_DispatchNo varchar(30),"//派工单号
				"	PCS_MO varchar(25),"//工单号
				"	PCS_ItemNO varchar(25),"//产品编号
				"	PCS_ItemName varchar(55),"//产品描述
				"	PCS_MouldNo varchar(25),"//模具编号
				"	PCS_DispatchQty integer,"//派工数量
				"	PCS_SocketNum1 integer,"//模具可用模穴数
				"	PCS_SocketNum2 integer,"//产品可用模穴数
				"	PCS_FitMachineMin integer,"//模具适应机型吨位最小值
				"	PCS_FitMachineMax integer,"//模具适应机型吨位最大值
				"	PCS_BadQty integer,	"//次品总数
				"	PCS_BadData varchar,"//每件产品的次品数据（对应次品原因的数据）
				"	PCS_AdjNum integer,"//调机数
				"   TotalOkNum integer,"//良品数
				"	CurClassProductNo integer,"//本班产品数
				"	CurClassBadNo integer,"//本班次品数
                "	CurClassPolishNo integer,"//打磨数
                "	CurClassInspectNo integer,"//寻机数
				"	ADJDefCount integer,"	//调机次品数
				"ADJOKNum integer,"//调机良品数
				"ADJEmptyMoldNum integer," //调机空模数
				"ADJDefList varchar,"  //调机中的次品列表
				"No1 integer,No2 Integer,Str1 varchar,Str2 varchar"//预留的
				")";
QString tb_EmployeeInfo = "CREATE TABLE IF NOT EXISTS tb_EmployeeInfo ("
//"Id integer primary key AutoIncrement,"
				"EmpNameCN varchar,"//员
				"IDCardNo varchar(15),"//卡号
				"EmpID varchar(25) primary key,"//员工工号 key
				"Post varchar(25),"//职位
				"picName varchar,"//照片名
				"IcCardRight varchar(20),"//权限
				"Notes varchar )";//备注

QString tb_Notebook = "CREATE TABLE IF NOT EXISTS tb_Notebook ("
		"id integer primary key AutoIncrement," //pk autoincrement
		"type varchar,"//类型
		"title varchar,"//标题
		"text varchar,"//XML 数据
		"result varchar,"//响应结果
		"addTime varchar,"//添加时间
		"upTime varchar)";//上传时间

QString tb_Material = "create TABLE IF NOT EXISTS tb_Material ("
		"id integer PRIMARY KEY autoincrement,"
		"DispatchNo varchar," // 派工单号
		"MaterialNO varchar,"// 原料编号
		"MaterialName varchar,"// 原料名称
		"BatchNO varchar,"// 原料批次号 //以下与投料有关
		"FeedingQty varchar,"// 原料投料数量
		"FeedingTime varchar)";// 原料投料时间
//
//QString tb_BrushCard = "CREATE TABLE IF NOT EXISTS tb_BrushCard ("
//		" id integer primary key AutoIncrement," //id PRIMARY KEY autoincrement
//		" DispatchNo varchar(25),"//	20	ASC	派工单号
//		" DispatchPrior varchar(30),"//	30	ASC	派工项次
//		" ProcCode varchar(22),"//	20	ASC	工序代码
//		" StaCode varchar(15),"//	10	ASC	站别代码
//		" CardID varchar(10),"//	10	HEX	卡号
//		" CardType integer,"//	1	HEX	刷卡原因编号
//		" CardDate varchar,"//	6	HEX	刷卡数据产生时间
//		" IsBeginEnd integer,"//	1	HEX	刷卡开始或结束标记，0表示开始，1表示结束。
//		" CarryDataLen integer,"//	2	HEX	刷卡携带数据长度（N）
//		" CarryData varchar)";//	N	HEX	刷卡携带数据内容
//
//QString tb_DefectiveInfo = "CREATE TABLE IF NOT EXISTS tb_DefectiveInfo("
//		"id integer PRIMARY KEY autoincrement," //integer PRIMARY KEY autoincrement
//		"DispatchNo varchar(25),"//	20	ASC	派工单号
//		"DispatchPrior varchar,"//	30	ASC	派工项次
//		"ProcCode varchar,"//	20	ASC	工序代码
//		"StaCode varchar,"//	10	ASC	站别代码
//		"CardID varchar,"//	10	HEX	卡号
//		"CardDate varchar,"//	6	HEX	次品数据产生时间
//		"Status integer,"//	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
//		"MultiNum integer,"//	1	HEX	总件数N（0< N <= 100）
//		"BadReasonNum integer,"//	1	HEX	次品原因总数M（0< M <= 100）
//		"Data varchar)";//	N*Bad_Data_SIZE	HEX	N件产品的次品数据
//
//QString tb_ADJMachine = "CREATE TABLE IF NOT EXISTS tb_ADJMachine ("
//		"id integer PRIMARY KEY autoincrement," //integer PRIMARY KEY autoincrement
//		" DispatchNo varchar(25),"//	20	ASC	派工单号
//		" DispatchPrior varchar(35),"//	30	ASC	派工项次
//		" ProcCode varchar(25),"//	20	ASC	工序代码
//		" StaCode varchar(15),"//	10	ASC	站别代码
//		" StartCardID varchar(15),"//	10	ASC	开始调机卡号
//		" StartTime varchar(20),"//	6	HEX	开始调机时间
//		" EndCardID varchar(10),"//	10	ASC	结束调机卡号
//		" EndTime varchar(20),"//	6	HEX	结束调机时间
//		" CardType integer,"//	1	HEX	调机原因编号
//		" MultiNum integer,"//	1	HEX	总件数N（0< N <= 100）
//		" Data varchar)";//	N*Adjust_Data_SIZE	HEX	N件产品的调机数据
//
//QString tb_OtherSetInfo = "CREATE TABLE IF NOT EXISTS tb_OtherSetInfo("
//		"id integer PRIMARY KEY autoincrement,"
//		"DataType integer,"
//		"FDAT_Data varchar"
//		")";

///////////////////////////////////////////////////////////
SQLiteBaseHelper SQLiteBaseHelper::instance;

SQLiteBaseHelper::SQLiteBaseHelper()
{
}

void SQLiteBaseHelper::onCreate(QSqlDatabase& db)
{
	if (db.isOpen())
	{
		exec(TB_VERSION_SQL);
		exec(tb_order);
		exec(tb_orderBoy);
		exec(tb_EmployeeInfo);
		exec(tb_Notebook);
		exec(tb_Material);
	}
}

void SQLiteBaseHelper::onUpgrade(QSqlDatabase& db, int oldVersion,
		int newVersion)
{
	if (db.isOpen())
	{
		oldVersion = newVersion;
	}
}

SQLiteBaseHelper &SQLiteBaseHelper::getInstance()
{
	return instance;
}

} /* namespace sqlite */

