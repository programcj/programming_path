#include "sqlitebasehelper.h"
#include "mesdispatchorder.h"
#include "mesproducteds.h"

//TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime'))
//插入不写，就有默认值，搜索就以这个时间排序,就是接收时间了,次单先发呢？
//////
namespace sqlite {

SQLiteBaseHelper *SQLiteBaseHelper::instance;

///
/// \brief SQLiteBaseHelper::SQLiteBaseHelper
///
SQLiteBaseHelper::SQLiteBaseHelper()
{
    instance=this;
}

void SQLiteBaseHelper::onCreate(QSqlDatabase& db)
{
    if (db.isOpen())
    {
        exec(TB_VERSION_SQL);
        ///////////////////////////////////////////////////////
        MESDispatchOrder::onCreate(this);
        MESProducteds::onCreate(this);

       // exec(TB_CREATE_ORDER);
       // exec(TB_CREATE_ORDER_BODY);
       // exec(TB_CREATE_PRODUCTED);
    }
}

void SQLiteBaseHelper::onUpgrade(QSqlDatabase& db, int oldVersion,
                                 int newVersion)
{
    if (db.isOpen())
    {
        oldVersion = newVersion;
    }
}

SQLiteBaseHelper &SQLiteBaseHelper::getInstance()
{
    return *instance;
}

bool sqlite::SQLiteBaseHelper::selectOne(QString sql, QJsonObject &json)
{

        QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());

        if (query.exec(sql))
        {
            if (query.next())
            {
                int size = query.record().count();
                QSqlRecord record = query.record();

                for(int i=0;i<size;i++){
                    QVariant value = query.value(i);
                    if (value.isNull())
                        continue;
                    ///qDebug() <<QString("%1=%2>>").arg(record.fieldName(i)).arg(value.toString());
                    json.insert(record.fieldName(i), value.toString());
                }
                ///qDebug() <<"\r\n";
                return true;
            }
            return false;
        }
        qDebug() << "SQL-ERR:" <<sql;
        return false;
    }
}
