#ifndef SQLITEOPENHELPER_H
#define SQLITEOPENHELPER_H

#include <QDebug>
#include <QtSql>
#include <QString>

namespace sqlite {

class TBVersion //用于版本管理的
{
    QString KeyName; // varchar(30),
    QString TheVersion; // varchar(30),
    int TheKey; // Integer
public:
    TBVersion() :
            KeyName(""), TheVersion(""), TheKey(0)
    {

    }

    static bool query(const QSqlDatabase &m_db, TBVersion &v, const QString &KeyName)
    {
        QSqlQuery query(m_db);
        //KeyName varchar(30), TheVersion varchar(30), TheKey Integer
        query.prepare(
                "SELECT KeyName,TheVersion,TheKey from tb_version where KeyName=?");
        query.addBindValue(KeyName);
        if (query.exec())
        {
            if (query.next())
            {
                v.setKeyName(query.value(0).toString());
                v.setTheVersion(query.value(1).toString());
                v.setTheKey(query.value(2).toInt());
                return true;
            }
        }
        return false;
    }

    static bool save(const QSqlDatabase &m_db, TBVersion &v)
    {
        QSqlQuery query(m_db);
        query.prepare(
                "REPLACE INTO tb_version (KeyName,TheVersion,TheKey) VALUES (?,?,?)");
        query.addBindValue(v.getKeyName());
        query.addBindValue(v.getTheVersion());
        query.addBindValue(v.getTheKey());
        return query.exec();
    }

    const QString& getKeyName() const
    {
        return KeyName;
    }

    void setKeyName(const QString& keyName)
    {
        KeyName = keyName;
    }

    int getTheKey() const
    {
        return TheKey;
    }

    void setTheKey(int theKey)
    {
        TheKey = theKey;
    }

    const QString& getTheVersion() const
    {
        return TheVersion;
    }

    void setTheVersion(const QString& theVersion)
    {
        TheVersion = theVersion;
    }
};

class SQLiteOpenHelper //数据库操作功能
{
    QSqlDatabase m_db;
    QString name;
    int version;

public:
    static QString TB_VERSION_SQL;

    SQLiteOpenHelper();

    bool init(const QString &name, int version);
    void close();

    virtual void onCreate(QSqlDatabase &db) =0;
    virtual void onUpgrade(QSqlDatabase &db, int oldVersion, int newVersion)=0;
    virtual ~SQLiteOpenHelper();

    const QSqlDatabase& getDB() const;
    bool transaction();
    bool commit();

    bool exec(const QString &sqlStr);

    bool exec(QSqlQuery &query, const QString &sql="", const QString &file = "",
              const QString &function = "", int line = 0);

    //版本检查,相等返回true
    bool versionCheck(const QString &key,const QString &verStr);
    //版本保存
    bool versionSave(const QString &key,const QString &verStr);
    //返回版本
    QString getVersion(const QString &key);

    template<typename T>
    bool selectList(QList<T> &resultList, const QString &table,
                    const QString &columns = "*", const QString &selection = "",
                    const QString &orderBy = "") const
    {
        QSqlQuery query(m_db);
        QString sql;
        sql = "SELECT " + columns + " from " + table;
        if (selection.length() > 0)
            sql += " WHERE " + selection;
        if (orderBy.length() > 0)
            sql += " order by " + orderBy;

        if (query.exec(sql))
        {
            while (query.next())
            {
                T producted;
                producted.read(query);
                resultList.append(producted);
            }
            return true;
        }
        //logErr(QString("DBErr:%1,%1").arg(sql).arg(query.lastError().text()));
        return false;
    }

    /**
     * 选择数据库中的一条记录
     * vlaue : 你所要保存的实体类
     * columns: 列名称
     * selection:选择参数
     * orderBy: 用于排序按照升序对记录进行排序
     * limit: 用于选择的
     */
    
};

}

#endif // SQLITEOPENHELPER_H
