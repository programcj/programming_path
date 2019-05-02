#include "sqliteopenhelper.h"

QString sqlite::SQLiteOpenHelper::TB_VERSION_SQL="CREATE TABLE IF NOT EXISTS tb_version (KeyName varchar(30) Primary Key, TheVersion varchar(30), TheKey Integer)";

bool sqlite::SQLiteOpenHelper::init(const QString& name, int version) {
    this->name = name;
    this->version = version;
    m_db = QSqlDatabase::addDatabase("QSQLITE", name);
    m_db.setDatabaseName(name);
    if(m_db.open())
    {
        onCreate(m_db);
        return true;
    }

    /////////////////////////////////////////////////
    //qDebug()<<" 数据库打开出错：" + m_db.lastError().text());
    return false;
}

bool sqlite::SQLiteOpenHelper::transaction()
{
    return m_db.transaction();
}

bool sqlite::SQLiteOpenHelper::commit()
{
    return m_db.commit();
}

bool sqlite::SQLiteOpenHelper::exec(const QString& sqlStr)
{
    QSqlQuery query(m_db);
    if (query.exec(sqlStr))
        return true;
    qDebug() << QString("DBErr:[%1]%2").arg(sqlStr).arg(query.lastError().text());
    return false;
}

sqlite::SQLiteOpenHelper::SQLiteOpenHelper()
{

}

sqlite::SQLiteOpenHelper::~SQLiteOpenHelper()
{
    if(m_db.isOpen()){
        m_db.close();
    }
}

const QSqlDatabase& sqlite::SQLiteOpenHelper::getDB() const
{
    return m_db;
}

bool sqlite::SQLiteOpenHelper::exec(QSqlQuery& query,const QString &sql,  const QString& file,
        const QString& function, int line)
{
    if (query.exec())
        return true;

    qDebug() << QString("[%1,%2,%3]:%4,err_db:%5").arg(file).arg(function)
            .arg(line).arg(sql).arg(query.lastError().text());
    return false;
}

bool sqlite::SQLiteOpenHelper::versionCheck(const QString &key, const QString &verStr)
{
    sqlite::TBVersion version;
    if (sqlite::TBVersion::query(getDB(), version, key))
        if (version.getTheVersion() == verStr)
            return true;
    return false;
}

bool sqlite::SQLiteOpenHelper::versionSave(const QString &key, const QString &verStr)
{
    sqlite::TBVersion version;
    version.setKeyName(key);
    version.setTheVersion(verStr);
    return sqlite::TBVersion::save(getDB(),version);
}

QString sqlite::SQLiteOpenHelper::getVersion(const QString &key)
{
    sqlite::TBVersion version;
    if (sqlite::TBVersion::query(getDB(), version, key))
        return version.getTheVersion();
    return "";
}

void sqlite::SQLiteOpenHelper::close()
{
    m_db.close();
}
