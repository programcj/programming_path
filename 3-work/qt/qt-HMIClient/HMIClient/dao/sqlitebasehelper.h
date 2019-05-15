#ifndef SQLITEBASEHELPER_H
#define SQLITEBASEHELPER_H

#include "sqliteopenhelper.h"
#include "../public.h"

#define TABLE_DISPATCH_ORDER        "Mes_DispatchOrder"
#define TABLE_FIELD_CREATED_TIME    "CreatedTime"
#define TABLE_FIELD_MO_DispatchNo "MO_DispatchNo"

#define TABLE_DORDER_ITEM   "Mes_DispatchOrder_Item"

namespace sqlite {

class SQLiteBaseHelper: public SQLiteOpenHelper {

public:
    SQLiteBaseHelper();
    virtual void onCreate(QSqlDatabase &db);
    virtual void onUpgrade(QSqlDatabase &db, int oldVersion, int newVersion);

    static SQLiteBaseHelper *instance;
    static SQLiteBaseHelper &getInstance();

    bool selectOne(QString sql,QJsonObject &json) ;
};

}

#endif // SQLITEBASEHELPER_H
