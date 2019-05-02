/*
 * Notebook.h
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#ifndef NOTEBOOK_H_
#define NOTEBOOK_H_

#include "absentity.h"
#include "UpXMLData/ADJMachine.h"
#include "UpXMLData/BrushCard.h"
#include "UpXMLData/DefectiveInfo.h"
#include "UpXMLData/OtherSetInfo.h"
#include "UpXMLData/QualityRegulate.h"

namespace entity {

/*
 *事情记录表，数据上传
 */
class Notebook: public AbsEntity {
	int id; //pk autoincrement
	QString type; //  类型
	QString title; // 标题
	QString text; //  XML 数据
	QString result; // 响应结果
	QString addTime; //添加时间
	QString upTime; //上传时间

DECLARE_COLUMNS_MAP()
	;

	void setText(const QString& text) {
		this->text = text;
	}
public:
	Notebook();
	Notebook(const QString &title, AbsPropertyBase &data);

	virtual bool subimt();
	virtual bool remove();
	bool insert();

    static int count(const QString &title);
    static int count();
public:
	//get set
	const QString& getAddTime() const {
		return addTime;
	}

	void setAddTime(const QString& addTime) {
		this->addTime = addTime;
	}

	int getId() const {
		return id;
	}

	void setId(int id) {
		this->id = id;
	}

	const QString& getResult() const {
		return result;
	}

	void setResult(const QString& result) {
		this->result = result;
	}

	const QString& getText() const {
		return text;
	}

	void setTextAbsXML(AbsPropertyBase &data) {
		setType(data.getClassName());
		setText(PropertyBaseToXML(data).toString());
	}

	const QString& getTitle() const {
		return title;
	}

	void setTitle(const QString& title) {
		this->title = title;
	}

	const QString& getType() const {
		return type;
	}

	void setType(const QString& type) {
		this->type = type;
	}

	const QString& getUpTime() const {
		return upTime;
	}

	void setUpTime(const QString& upTime) {
		this->upTime = upTime;
	}


};

}
/* namespace entity */

#endif /* NOTEBOOK_H_ */
