#include "mesproducteds.h"

static QString TB_CREATE_PRODUCTED ="CREATE TABLE IF NOT EXISTS Mes_Producteds ("
        "ID integer PRIMARY KEY autoincrement,"
        "TagType integer,"
        "Msg varchar,"
        "CreatedTime TIMESTAMP NOT NULL DEFAULT (DATETIME ('now', 'localtime'))"
        ")";

MESProducteds::MESProducteds()
{
}

void MESProducteds::onCreate(sqlite::SQLiteBaseHelper *dbHelp)
{
    dbHelp->exec(TB_CREATE_PRODUCTED);
}

void MESProducteds::onAppendMsg(int tag, QJsonObject &json)
{
    QString sql="insert into Mes_Producteds (TagType,Msg) values (?,?)";
    QString msg=QString(QJsonDocument(json).toJson());

    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    query.prepare(sql);
    query.bindValue(0,tag);
    query.bindValue(1,msg);
    if (false == sqlite::SQLiteBaseHelper::getInstance().exec(query, sql, __FILE__, __FUNCTION__, __LINE__)){
        qDebug() << "添加失败";
    }
}

int MESProducteds::onCountMsg()
{
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    if(query.exec("select count(*) from Mes_Producteds")){
        if(query.next()){
            return query.value(0).toInt();
        }
        return 0;
    }
    return 0;
}

bool MESProducteds::onFirstMsg(MESProducteds &msg)
{
    QSqlQuery query(sqlite::SQLiteBaseHelper::getInstance().getDB());
    if(query.exec("select ID,TagType,Msg,CreatedTime from Mes_Producteds")){
        if(query.next()) {
                msg.ID=query.value(0).toInt();
                msg.TagType=query.value(1).toInt();
                msg.Msg=query.value(2).toString();
                msg.CreatedTime =query.value(3).toString();
                return true;
        }
        return false;
    }
    return false;
}

void MESProducteds::onDelete(int pk)
{
    QString sql=QString("delete from Mes_Producteds where ID=%1").arg(pk);
    sqlite::SQLiteBaseHelper::getInstance().exec(sql);
}
