/*
 * MESLog.h
 *
 *  Created on: 2015年1月22日
 *      Author: cj
 */

#ifndef MESLOG_H_
#define MESLOG_H_

#include <QtCore>
#include <QFile>

class MESLogContext
{
public:
    MESLogContext() :
        line(0), file(0), function(0)
    {
    }
    MESLogContext(const char *fileName, int lineNumber,
                  const char *functionName) :
        line(lineNumber), file(fileName), function(functionName)
    {
    }

    int line;
    const char *file;
    const char *function;
};

class MESLog
{
public:
    enum Level
    {
        asALL, //ALL Level是最低等级的，用于打开所有日志记录。
        asOFF, //OFF Level是最高等级的，用于关闭所有日志记录。
        asDebug, //DEBUG Level指出细粒度信息事件对调试应用程序是非常有帮助的
        asInfo, //INFO level表明 消息在粗粒度级别上突出强调应用程序的运行过程
        asWarn, //WARN level表明会出现潜在错误的情形。
        asErr, //ERROR level指出虽然发生错误事件，但仍然不影响系统的继续运行。
        asFatal //level指出每个严重的错误事件将会导致应用程序的退出。
    };

    static void initPaths(const QString &path); //初始化日志目录
    static void setLevel(int level); //设定日志级别
    static bool checkLogFile(); //检查并清除多余的日志文件

    MESLog();
    MESLog(const char *file, int line, const char *function,Level l);
    ~MESLog();

    ///////////////////////////////////////////////////////////
    void debug(const QString &str);
    void info(const QString &str);
    void warn(const QString &str);
    void err(const QString &str);
    void fatal(const QString &str);
    ///////////////////////////////////////////////////////////
    MESLog & debug();
    MESLog & info();
    MESLog & warn();
    MESLog & err();
    MESLog & fatal();

    inline MESLog &space() { myspace = true; ts << ' '; return *this; }
    inline MESLog &nospace() { myspace = false; return *this; }
    inline MESLog &maybeSpace() { if (myspace) ts << ' '; return *this; }

    inline MESLog &operator<<(QChar t) { ts << '\'' << t << '\''; return maybeSpace(); }
    inline MESLog &operator<<(QBool t) { ts << (bool(t != 0) ? "true" : "false"); return maybeSpace(); }
    inline MESLog &operator<<(bool t) { ts << (t ? "true" : "false"); return maybeSpace(); }
    inline MESLog &operator<<(char t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(signed short t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(unsigned short t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(signed int t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(unsigned int t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(signed long t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(unsigned long t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(qint64 t)
    { ts << QString::number(t); return maybeSpace(); }
    inline MESLog &operator<<(quint64 t)
    { ts << QString::number(t); return maybeSpace(); }
    inline MESLog &operator<<(float t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(double t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(const char* t) { ts << QString::fromAscii(t); return maybeSpace(); }
    inline MESLog &operator<<(const QString & t) { ts << '\"' << t  << '\"'; return maybeSpace(); }
    inline MESLog &operator<<(const QStringRef & t) { return operator<<(t.toString()); }
    inline MESLog &operator<<(const QLatin1String &t) { ts << '\"'  << t.latin1() << '\"'; return maybeSpace(); }
    inline MESLog &operator<<(const QByteArray & t) { ts  << '\"' << t << '\"'; return maybeSpace(); }
    inline MESLog &operator<<(const void * t) { ts << t; return maybeSpace(); }
    inline MESLog &operator<<(QTextStreamFunction f) {
        ts << f;
        return *this;
    }

    inline MESLog &operator<<(QTextStreamManipulator m)
    { ts << m; return *this; }

private:
    static QString m_pathLogs; //日志目录
    static int defLevel; //默认日志级别设定条件

    void write(const QString &str);

    bool myspace;
    bool message_output;

    QTime time;
    QFile file;
    QTextStream ts;
    MESLog::Level level;
    MESLogContext context;
};

/**
 * 日志打印格式
 */
#define logDebug MESLog(__FILE__, __LINE__, __FUNCTION__,MESLog::asDebug).debug
#define logInfo MESLog(__FILE__, __LINE__, __FUNCTION__,MESLog::asInfo).info
#define logWarn MESLog(__FILE__, __LINE__, __FUNCTION__,MESLog::asWarn).warn
#define logErr MESLog(__FILE__, __LINE__, __FUNCTION__,MESLog::asErr).err
#define logFatal  MESLog(__FILE__, __LINE__, __FUNCTION__,MESLog::asFatal).fatal

#endif /* MESLOG_H_ */
