/*
 * Notebook.cpp
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#include "Notebook.h"
#include "../SQLiteBaseHelper.h"
#include "../SQLiteProductedHelper.h"
#include "../../Unit/Tool.h"

namespace entity
{

BEGIN_COLUMNS_MAP(tb_Notebook, Notebook)
COLUMNS_INT	( id, &Notebook::setId) //pk autoincrement
	COLUMNS_STR( type, &Notebook::setType)//  类型
	COLUMNS_STR( title, &Notebook::setTitle)// 标题
	COLUMNS_STR( text, &Notebook::setText)//  XML 数据
	COLUMNS_STR( result, &Notebook::setResult)// 响应结果
	COLUMNS_STR( addTime, &Notebook::setAddTime)//添加时间
	COLUMNS_STR( upTime, &Notebook::setUpTime)//上传时间
	END_COLUMNS_MAP()

Notebook::Notebook()
{
	id = 0;
}

Notebook::Notebook(const QString &title, AbsPropertyBase &data)
{
	setTitle(title);
	setTextAbsXML(data);
	setAddTime(unit::Tool::GetCurrentDateTimeStr());
}

bool Notebook::subimt()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("REPLACE INTO tb_Notebook ("
			"id,type,"
			"title,"
			"text,"
			"result,"
			"addTime,"
			"upTime"
			") values (?,?,?,?,?,?,?)");
	query.addBindValue(id);
	query.addBindValue(type);
	query.addBindValue(title);
	query.addBindValue(text);
	query.addBindValue(result);
	query.addBindValue(addTime);
	query.addBindValue(upTime);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query);
}

bool Notebook::remove()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("delete from tb_Notebook where id = ?");
	query.bindValue(0, id);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query, __FILE__,
			__FUNCTION__, __LINE__);
}

bool Notebook::insert()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("insert into tb_Notebook ("
			"type,"
			"title,"
			"text,"
			"result,"
			"addTime,"
			"upTime"
			") values (?,?,?,?,?,?)");
	//query.addBindValue(id);
	query.addBindValue(type);
	query.addBindValue(title);
	query.addBindValue(text);
	query.addBindValue(result);
	query.addBindValue(addTime);
	query.addBindValue(upTime);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query);
}

int Notebook::count(const QString &title)
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("select count(*) from tb_Notebook where title=?");
	query.addBindValue(title);
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query))
	{
		if (query.next())
			return query.value(0).toInt();
	}
	return 0;
}

int Notebook::count()
{
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("select count(*) from tb_Notebook");
	if (sqlite::SQLiteBaseHelper::getInstance().exec(query))
		if (query.next())
			return query.value(0).toInt();
	return 0;
}
} /* namespace entity */

