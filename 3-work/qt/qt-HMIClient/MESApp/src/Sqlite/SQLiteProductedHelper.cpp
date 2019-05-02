/*
 * SQLiteProductedHelper.cpp
 *
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#include "SQLiteProductedHelper.h"

namespace sqlite
{
SQLiteProductedHelper SQLiteProductedHelper::instance;

QString tb_Producted = "create TABLE IF NOT EXISTS tb_Producted ("
		"id	integer PRIMARY KEY autoincrement," //	id pk auto
		"DispatchNo varchar,"//	派工单号
		"DispatchPrior varchar,"//派工项次
		"ProcCode varchar,"///		工序代码
		"StaCode varchar,"//		站别代码
		"EndCycle varchar,"//		生产数据产生时间
		"TemperValue varchar,"//	N路温度值 List<int>
		"MachineCycle integer,"//	机器周期
		"FillTime	integer,"//	填充时间
		"CycleTime	integer,"//	成型周期
		"TotalNum	integer,"//	模次
		"KeepPress	varchar,"//	100个压力点 List<int>
		"StartCycle	varchar"//	生产数据开始时间
		")";

QString tb_Producted_bak = "create TABLE IF NOT EXISTS tb_Producted_bak ("
		"id	integer PRIMARY KEY autoincrement," //	id pk auto
		"DispatchNo varchar,"//	派工单号
		"DispatchPrior varchar,"//派工项次
		"ProcCode varchar,"///		工序代码
		"StaCode varchar,"//		站别代码
		"EndCycle varchar,"//		生产数据产生时间
		"TemperValue varchar,"//	N路温度值 List<int>
		"MachineCycle integer,"//	机器周期
		"FillTime	integer,"//	填充时间
		"CycleTime	integer,"//	成型周期
		"TotalNum	integer,"//	模次
		"KeepPress	varchar,"//	100个压力点 List<int>
		"StartCycle	varchar"//	生产数据开始时间
		")";

SQLiteProductedHelper::SQLiteProductedHelper()
{

}

void SQLiteProductedHelper::onCreate(QSqlDatabase& db)
{
	if (db.isOpen())
	{
		if (!exec(tb_Producted))
            logErr("err tb_Producted");
		if (!exec(tb_Producted_bak))
            logErr("err tb_Producted");
	}
}

void SQLiteProductedHelper::onUpgrade(QSqlDatabase& db, int oldVersion,
		int newVersion)
{
	if (db.isOpen())
	{
		oldVersion = newVersion;
	}
}

SQLiteProductedHelper& SQLiteProductedHelper::getInstance()
{
	return instance;
}

} /* namespace mes_protocol */
