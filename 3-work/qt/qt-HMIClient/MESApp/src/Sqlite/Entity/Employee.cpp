#include "Employee.h"
#include "../SQLiteBaseHelper.h"
#include "../SQLiteProductedHelper.h"

namespace entity {

BEGIN_COLUMNS_MAP(tb_EmployeeInfo, Employee)
//COLUMNS_INT	(Id, &Employee::setId )
COLUMNS_STR	(EmpNameCN, &Employee::setEmpNameCN)
	COLUMNS_STR(IDCardNo, &Employee::setIDCardNo)
	COLUMNS_STR(EmpID, &Employee::setEmpID)
	COLUMNS_STR(Post, &Employee::setPost)
	COLUMNS_STR(PicName, &Employee::setPicName)
	COLUMNS_STR(IcCardRight, &Employee::setIcCardRight)
	COLUMNS_STR(Notes, &Employee::setNotes)
	END_COLUMNS_MAP()

Employee::Employee() :
		EmpNameCN(""), IDCardNo(""), EmpID(""), Post(""), PicName(""), IcCardRight(
				""), Notes("") {

}

bool Employee::subimt() {
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare(
			"REPLACE INTO tb_EmployeeInfo (EmpNameCN,IDCardNo,EmpID,Post,PicName,IcCardRight,Notes) VALUES (?,?,?,?,?,?,?)");
	query.addBindValue(EmpNameCN);
	query.addBindValue(IDCardNo); //卡号
	query.addBindValue(EmpID);    //工号
	query.addBindValue(Post);
	query.addBindValue(PicName);
	query.addBindValue(IcCardRight);
	query.addBindValue(Notes);
	return sqlite::SQLiteBaseHelper::getInstance().exec(query);
}

bool Employee::remove() {
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("delete from tb_EmployeeInfo where EmpID = ?");
	query.bindValue(0, EmpID);
	return query.exec();
}

bool Employee::queryIDCardNo(Employee& employee, const QString& IDCardNo) {
	QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
	query.prepare("SELECT * from tb_EmployeeInfo where IDCardNo=?");
	query.bindValue(0, IDCardNo);
	if (query.exec()) {
		if (query.next()) {
			employee.read(query);
			return true;
		}
	}
	return false;
}

int Employee::QureyAllEmployee(QList<Employee> & listworker,
		const QSqlDatabase &db) {
	QSqlQuery query(db);
	if (query.exec("SELECT * from tb_EmployeeInfo")) {
		while (query.next()) {
			Employee worker;
			worker.read(query);
			listworker.append(worker);
		}
	}
	return listworker.count();
}

}

