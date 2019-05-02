/*
 * Material.cpp
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#include "Material.h"
#include "../SQLiteBaseHelper.h"
#include "../SQLiteProductedHelper.h"

namespace entity {

BEGIN_COLUMNS_MAP(tb_Material,Material)
COLUMNS_INT	(id,&Material::setId) //  // primary key
	COLUMNS_STR( DispatchNo,&Material::setDispatchNo)//		派工单号
	COLUMNS_STR( MaterialNO,&Material::setMaterialNo)//		原料编号
	COLUMNS_STR( MaterialName,&Material::setMaterialName)//	原料名称
	COLUMNS_STR( BatchNO,&Material::setBatchNo)//			原料批次号 //以下与投料有关
	COLUMNS_STR( FeedingQty,&Material::setFeedingQty)//		原料投料数量
	COLUMNS_STR( FeedingTime,&Material::setFeedingTime)//	原料投料时间
	END_COLUMNS_MAP()

bool Material::subimt() {
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"SELECT id from tb_Material where DispatchNo=? and MaterialNO=?");
	query.addBindValue(DispatchNo);
	query.addBindValue(MaterialNO);
	if (query.exec()) {
		if (query.next()) {
			setId(query.value(0).toInt());
		} else {
			setId(-1);
		}
	} else {
		setId(-1);
		//qDebug() << query.lastError().text();
	}
	if (-1 != getId()) {
		query.prepare("REPLACE INTO tb_Material("
				"id,DispatchNo,MaterialNO,MaterialName,"
				"BatchNO,FeedingQty,FeedingTime)"
				" values (?,?,?,?,?,?,?)");
		query.addBindValue(id);
	} else
		query.prepare("REPLACE INTO tb_Material("
				"DispatchNo,MaterialNO,MaterialName,"
				"BatchNO,FeedingQty,FeedingTime)"
				" values (?,?,?,?,?,?)");

	query.addBindValue(DispatchNo);
	query.addBindValue(MaterialNO);
	query.addBindValue(MaterialName);
	query.addBindValue(BatchNO);
	query.addBindValue(FeedingQty);
	query.addBindValue(FeedingTime);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query,__FILE__,__FUNCTION__,__LINE__);
}

bool Material::remove() {
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("delete from tb_Material where id=? ");
	query.bindValue(0, id);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query);
}

//以id 排序从小到大
bool Material::queryAll(QList<Material>& list, const QString &MoDispatchNo) {
	QString selection = "DispatchNo='" + MoDispatchNo + "'";
	return sqlite::SQLiteBaseHelper::getInstance().selectList(list,
			Material::GetThisTableName(), "*", selection, "id");
}

} /* namespace Entity */
