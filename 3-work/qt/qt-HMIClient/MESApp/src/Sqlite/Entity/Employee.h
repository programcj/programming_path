#ifndef __Employee_H__
#define __Employee_H__

#include "absentity.h"

namespace entity
{

class Employee: public AbsEntity
{
private:
	//int Id;
	QString EmpNameCN;
	QString IDCardNo;
	QString EmpID;
	QString Post;
	QString PicName;
	QString IcCardRight;
	QString Notes;

DECLARE_COLUMNS_MAP()
	;

public:
	Employee();

	virtual bool subimt();
	virtual bool remove();

	//工号查员工
	static bool queryIDCardNo(Employee &employee,const QString &IDCardNo);

	//查询所有员工信息
	int QureyAllEmployee(QList<Employee> & listworker, const QSqlDatabase &db);

	///////////////////////////////////////////////////////////////////////////////////////////////
//	int getId() const {
//		return Id;
//	}
//
//	void setId(int id) {
//		this->Id = id;
//	}

	QString getIDCardNo() const
	{
		return IDCardNo;
	}

	void setIDCardNo(const QString & IDCardNo)
	{
		this->IDCardNo = IDCardNo;
	}

	QString getEmpID()
	{
		return EmpID;
	}

	void setEmpID(const QString & empID)
	{
		this->EmpID = empID;
	}
	QString getPost()
	{
		return Post;
	}
	void setPost(const QString & post)
	{

		this->Post = post;
	}

	QString getPicName()
	{
		return PicName;
	}

	void setPicName(const QString& picname)
	{
		this->PicName = picname;
	}

	QString getIcCardRight()
	{
		return IcCardRight;
	}

	void setIcCardRight(const QString& IcCardRight)
	{
		this->IcCardRight = IcCardRight;
	}
	QString getEmpNameCN() const
	{
		return EmpNameCN;
	}

	void setEmpNameCN(const QString & name)
	{
		this->EmpNameCN = name;
	}

	QString getNotes() const
	{
		return Notes;
	}

	void setNotes(const QString & text)
	{
		this->Notes = text;
	}
};
}

#endif // Employee_H
