/*
 * MESLog.cpp
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#include "MESLog.h"

#include <QString>
#include <QtCore>

QString MESLog::m_pathLogs; //日志目录
int MESLog::defLevel; //默认日志级别设定条件

#define S_DEBUG "DEBUG:"
#define S_ALL "ALL:"
#define S_INFO "INFO:"
#define S_WARN "WARN:"
#define S_ERR "ERR:"
#define S_FATAL "FATAL:"

void MESLog::write( const QString &string)
{
    QString str;

    switch (level)
    {
    case asALL: //ALL Level是最低等级的，用于打开所有日志记录。
        str = S_ALL;
        break;
    case asOFF: //OFF Level是最高等级的，用于关闭所有日志记录。
        str = "OFF:";
        return;
        break;
    case asDebug: //DEBUG Level指出细粒度信息事件对调试应用程序是非常有帮助的
        str = S_DEBUG;
        break;
    case asInfo: //INFO level表明 消息在粗粒度级别上突出强调应用程序的运行过程
        str = S_INFO;
        break;
    case asWarn: //WARN level表明会出现潜在错误的情形。
        str = S_WARN;
        break;
    case asErr: //ERROR level指出虽然发生错误事件，但仍然不影响系统的继续运行。
        str = S_ERR;
        break;
    case asFatal: // level指出每个严重的错误事件将会导致应用程序的退出。
        str = S_FATAL;
        break;
    default:
        return;
        break;
    }
    str+=QString("%1:%2:%3 [%4,%5:%6] %7 \r\n")
            .arg(time.hour()).arg(time.minute()).arg(time.second())
            .arg(context.file).arg(context.function).arg(context.line).arg(string);

    printf("%s",str.toAscii().data());

    if(this->file.isOpen())
    {
        ts << str;
    }
}

bool MESLog::checkLogFile()
{
    //检查多余的文件,并清除
    QDir dir(m_pathLogs);
    if (dir.exists())
    {
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        dir.setSorting(QDir::Name); //时间排序
        QFileInfoList list = dir.entryInfoList();
        int file_count = list.count();
        if (file_count > 7) //超过7天的就去掉
        {
            for (int i = 0; i < file_count - 7; i++)
            {
                QFileInfo file_info = list.at(i);
                logInfo(QString("删除多余日志:%1").arg(file_info.baseName()));
                QFile::remove(file_info.filePath()); //清除掉多余的日志
            }
        }
    }
    return true;
}

MESLog::MESLog()
{
    time = QTime::currentTime();
    level=asDebug;
    context.file=0;
}

void MESLog::initPaths(const QString &path)
{
    m_pathLogs = path;
}

void MESLog::setLevel(int level)
{
    defLevel=level;
}

static QMutex mutex;
///////////////////////////////////////////////////////////////////////////////////////////////
MESLog::MESLog(const char *file, int line, const char *function, MESLog::Level l):context(file, line, function),level(l)
{
    mutex.lock();
    time = QTime::currentTime();
    this->file.setFileName(m_pathLogs+QDateTime::currentDateTime().toString("log_yyyy-MM-dd.txt"));
    if( this->file.open(QIODevice::Append | QIODevice::WriteOnly) )
    {
        ts.setDevice(&this->file);
    }
}

MESLog::~MESLog()
{
    if(this->file.isOpen())
    {
        ts.flush();
        this->file.close();
    }
    mutex.unlock();
}

void MESLog::debug(const QString& string)
{   
    write(string);
}

void MESLog::info(const QString& string)
{
    write(string);
}

void MESLog::warn(const QString& string)
{
    write(string);
}

void MESLog::err(const QString& string)
{
    write(string);
}

void MESLog::fatal(const QString& string)
{
    write(string);
}
///////////////////////////////////////////////////////
MESLog &MESLog::debug()
{
    ts << S_DEBUG <<QString("%1:%2:%3 [%4,%5:%6] ")
          .arg(time.hour()).arg(time.minute()).arg(time.second())
          .arg(context.file).arg(context.function).arg(context.line);
    return *this;
}

MESLog &MESLog::info()
{
    ts << S_INFO <<QString("%1:%2:%3 [%4,%5:%6] ")
          .arg(time.hour()).arg(time.minute()).arg(time.second())
          .arg(context.file).arg(context.function).arg(context.line);

    return *this;
}

MESLog &MESLog::warn()
{
    ts << S_WARN <<QString("%1:%2:%3 [%4,%5:%6] ")
          .arg(time.hour()).arg(time.minute()).arg(time.second())
          .arg(context.file).arg(context.function).arg(context.line);
    return *this;
}

MESLog &MESLog::err()
{
    ts << S_ERR <<QString("%1:%2:%3 [%4,%5:%6] ")
          .arg(time.hour()).arg(time.minute()).arg(time.second())
          .arg(context.file).arg(context.function).arg(context.line);
    return *this;
}

MESLog &MESLog::fatal()
{
    ts << S_FATAL <<QString("%1:%2:%3 [%4,%5:%6] ")
          .arg(time.hour()).arg(time.minute()).arg(time.second())
          .arg(context.file).arg(context.function).arg(context.line);
    return *this;
}
