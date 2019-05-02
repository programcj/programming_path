/*
 * Material.h
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "absentity.h"

namespace entity {

/**
 * 原料信息
 */
class Material: public AbsEntity {
	int id; // primary key
	QString DispatchNo; //派工单号
	QString MaterialNO; //原料编号
	QString MaterialName; //原料名称
	//以下与投料有关
	QString BatchNO; //原料批次号
	QString FeedingQty; //原料投料数量
	QString FeedingTime; //原料投料时间

DECLARE_COLUMNS_MAP()
	;
public:
	virtual bool subimt();
	virtual bool remove();

	static bool queryAll(QList<Material> &list,const QString &MoDispatchNo);
	static bool query(const QString DispatchNo);
public:
	//原料批次号
	const QString& getBatchNo() const {
		return BatchNO;
	}
	//原料批次号
	void setBatchNo(const QString& batchNo) {
		BatchNO = batchNo;
	}
	//派工单号
	const QString& getDispatchNo() const {
		return DispatchNo;
	}
	//派工单号
	void setDispatchNo(const QString& dispatchNo) {
		DispatchNo = dispatchNo;
	}
	//原料投料数量
	const QString& getFeedingQty() const {
		return FeedingQty;
	}
	//原料投料数量
	void setFeedingQty(const QString& feedingQty) {
		FeedingQty = feedingQty;
	}
	//原料投料时间
	const QString& getFeedingTime() const {
		return FeedingTime;
	}
	//原料投料时间
	void setFeedingTime(const QString& feedingTime) {
		FeedingTime = feedingTime;
	}

	int getId() const {
		return id;
	}

	void setId(int id) {
		this->id = id;
	}
	//原料名称
	const QString& getMaterialName() const {
		return MaterialName;
	}
	//原料名称
	void setMaterialName(const QString& materialName) {
		MaterialName = materialName;
	}
	//		        原料编号
	const QString& getMaterialNo() const {
		return MaterialNO;
	}
	//		        原料编号
	void setMaterialNo(const QString& materialNo) {
		MaterialNO = materialNo;
	}
};

} /* namespace Entity */

#endif /* MATERIAL_H_ */
