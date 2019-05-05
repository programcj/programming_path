#ifndef SYSLOG_H
#define SYSLOG_H

#include <QFile>
#include <QObject>

class SysLog : public QObject
{
    Q_OBJECT
public:
    explicit SysLog(QObject *parent = 0);

signals:

public slots:

};

#endif // SYSLOG_H
