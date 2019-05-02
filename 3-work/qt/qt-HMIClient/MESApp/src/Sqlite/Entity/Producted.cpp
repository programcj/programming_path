/*
 * Producted.cpp
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#include "Producted.h"
#include "../SQLiteProductedHelper.h"

namespace entity
{

BEGIN_COLUMNS_MAP(tb_Producted,Producted)
COLUMNS_INT	(id,&Producted::setId)
	COLUMNS_STR (DispatchNo,&Producted::setDispatchNo)
	COLUMNS_STR(DispatchPrior,&Producted::setDispatchPrior)
	COLUMNS_STR(DispatchNo ,&Producted::setDispatchNo ) //		派工单号
	COLUMNS_STR(DispatchPrior ,&Producted::setDispatchPrior )//派工项次
	COLUMNS_STR(ProcCode ,&Producted::setProcCode )///			工序代码
	COLUMNS_STR(StaCode ,&Producted::setStaCode )//			站别代码
	COLUMNS_STR(EndCycle ,&Producted::setEndCycle )//			生产数据产生时间
	COLUMNS_STR(TemperValue ,&Producted::setTemperValue )//		N路温度值 List<int>
	COLUMNS_INT(MachineCycle ,&Producted::setMachineCycle )//		机器周期
	COLUMNS_INT(FillTime ,&Producted::setFillTime )//			填充时间
	COLUMNS_INT(CycleTime ,&Producted::setCycleTime )//成型周期
	COLUMNS_INT(TotalNum ,&Producted::setTotalNum )//模次
	COLUMNS_STR(KeepPress ,&Producted::setKeepPress )//100个压力点 List<int>
	COLUMNS_STR(StartCycle ,&Producted::setStartCycle )//生产数据开始时
	END_COLUMNS_MAP ()

bool Producted::subimt()
{
	QSqlQuery query(sqlite::SQLiteProductedHelper::getInstance().getDB());
	query.prepare("REPLACE INTO tb_Producted ("
	//"id	," //Y KEY autoincrement," //	id pk auto
					"DispatchNo	,"//派工单号
					"DispatchPrior,"//派工项次
					"ProcCode,"//		工序代码
					"StaCode,"//			站别代码
					"EndCycle,"//	生产数据产生时间
					"TemperValue,"//		N路温度值 List<int>
					"MachineCycle,"//机器周期
					"FillTime,"//	填充时间
					"CycleTime,"//	成型周期
					"TotalNum,"//模次
					"KeepPress,"//	100个压力点 List<int>
					"StartCycle"//
					") values "
					"(?,?,?,?,?,?,?,?,?,?,?,?)");
	//query.addBindValue(id); //Y KEY autoincrement," //	id pk auto
	query.addBindValue(DispatchNo); //派工单号
	query.addBindValue(DispatchPrior); //派工项次
	query.addBindValue(ProcCode); //		工序代码
	query.addBindValue(StaCode); //			站别代码
	query.addBindValue(EndCycle); //	生产数据产生时间
	QString _TemperValue;
	for (int i = 0; i < TemperValue.length(); i++)
	{
		QString v;
		_TemperValue += v.sprintf("%d", TemperValue[i]);
		if (i < TemperValue.length() - 1)
			_TemperValue += ",";
	}
	query.addBindValue(_TemperValue); //		N路温度值 List<int>
	query.addBindValue(MachineCycle); //机器周期
	query.addBindValue(FillTime); //	填充时间
	query.addBindValue(CycleTime); //	成型周期
	query.addBindValue(TotalNum); //模次
	QString _KeepPress;
	for (int i = 0; i < KeepPress.length(); i++)
	{
		QString v;
		_KeepPress += v.sprintf("%d", KeepPress[i]);
		if (i < KeepPress.length() - 1)
			_KeepPress += ",";
	}
	query.addBindValue(_KeepPress); //	100个压力点 List<int>
	query.addBindValue(StartCycle); //
	if (sqlite::SQLiteProductedHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
		return true;
	return false;
}

bool Producted::subimt_bak()
{
	QSqlQuery query(sqlite::SQLiteProductedHelper::getInstance().getDB());
	query.prepare("REPLACE INTO tb_Producted_bak ("
	//"id	," //Y KEY autoincrement," //	id pk auto
					"DispatchNo	,"//派工单号
					"DispatchPrior,"//派工项次
					"ProcCode,"//		工序代码
					"StaCode,"//			站别代码
					"EndCycle,"//	生产数据产生时间
					"TemperValue,"//		N路温度值 List<int>
					"MachineCycle,"//机器周期
					"FillTime,"//	填充时间
					"CycleTime,"//	成型周期
					"TotalNum,"//模次
					"KeepPress,"//	100个压力点 List<int>
					"StartCycle"//
					") values "
					"(?,?,?,?,?,?,?,?,?,?,?,?)");
	//query.addBindValue(id); //Y KEY autoincrement," //	id pk auto
	query.addBindValue(DispatchNo); //派工单号
	query.addBindValue(DispatchPrior); //派工项次
	query.addBindValue(ProcCode); //		工序代码
	query.addBindValue(StaCode); //			站别代码
	query.addBindValue(EndCycle); //	生产数据产生时间
	QString _TemperValue;
	for (int i = 0; i < TemperValue.length(); i++)
	{
		QString v;
		_TemperValue += v.sprintf("%d", TemperValue[i]);
		if (i < TemperValue.length() - 1)
			_TemperValue += ",";
	}
	query.addBindValue(_TemperValue); //		N路温度值 List<int>
	query.addBindValue(MachineCycle); //机器周期
	query.addBindValue(FillTime); //	填充时间
	query.addBindValue(CycleTime); //	成型周期
	query.addBindValue(TotalNum); //模次
	QString _KeepPress;
	for (int i = 0; i < KeepPress.length(); i++)
	{
		QString v;
		_KeepPress += v.sprintf("%d", KeepPress[i]);
		if (i < KeepPress.length() - 1)
			_KeepPress += ",";
	}
	query.addBindValue(_KeepPress); //	100个压力点 List<int>
	query.addBindValue(StartCycle); //
	if (sqlite::SQLiteProductedHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
		return true;
	return false;
}

bool Producted::remove()
{
	QSqlQuery query(sqlite::SQLiteProductedHelper::getInstance().getDB());
	query.prepare("delete from tb_Producted where ID=? ");
	query.bindValue(0, id);
	return sqlite::SQLiteProductedHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__);
}

bool Producted::allList(QList<Producted> &list)
{
	return sqlite::SQLiteProductedHelper::getInstance().selectList(list,
			"tb_Producted", "*", "");
}

int Producted::count_bak()
{
	QSqlQuery query(sqlite::SQLiteProductedHelper::getInstance().getDB());
	query.prepare("SELECT count(*) from tb_Producted_bak");
	if (sqlite::SQLiteProductedHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		if (query.next())
		{
			return query.value(0).toInt();
		}
	}
	return 0;
}

int Producted::count()
{
	QSqlQuery query(sqlite::SQLiteProductedHelper::getInstance().getDB());
	query.prepare("SELECT count(*) from tb_Producted");
	if (sqlite::SQLiteProductedHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__))
	{
		if (query.next())
		{
			return query.value(0).toInt();
		}
	}
	return 0;
}

} /* namespace entity */

