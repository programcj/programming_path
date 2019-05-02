/*
 * SQLiteProductedHelper.h
 *
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#ifndef SQLITEPRODUCTEDHELPER_H_
#define SQLITEPRODUCTEDHELPER_H_

#include "SQLiteOpenHelper.h"

namespace sqlite {

class SQLiteProductedHelper: public SQLiteOpenHelper {

public:
	SQLiteProductedHelper();
	virtual void onCreate(QSqlDatabase &db);
	virtual void onUpgrade(QSqlDatabase &db, int oldVersion, int newVersion);
	static SQLiteProductedHelper instance;
	static SQLiteProductedHelper &getInstance();
};

} /* namespace mes_protocol */

#endif /* SQLITEPRODUCTEDHELPER_H_ */
