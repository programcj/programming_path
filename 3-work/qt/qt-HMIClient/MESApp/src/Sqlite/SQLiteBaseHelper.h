/*
 * SQLiteBaseHelper.h
 *
 *  Created on: 2015年1月26日
 *      Author: cj
 */

#ifndef SQLITEBASEHELPER_H_
#define SQLITEBASEHELPER_H_

#include "SQLiteOpenHelper.h"

namespace sqlite {

class SQLiteBaseHelper: public SQLiteOpenHelper {
public:
	SQLiteBaseHelper();
	virtual void onCreate(QSqlDatabase &db);
	virtual void onUpgrade(QSqlDatabase &db, int oldVersion, int newVersion);

	static SQLiteBaseHelper instance;
	static SQLiteBaseHelper &getInstance();
};

}
/* namespace sqlite */

#endif /* SQLITEBASEHELPER_H_ */
