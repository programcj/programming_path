#ifndef QSYSTEMINFO_H
#define QSYSTEMINFO_H

#include <QtCore>

class QSystemInfo : public QThread
{
    Q_OBJECT
public:
    explicit QSystemInfo(QObject *parent = 0);
    ~QSystemInfo();

    int cpuUse;
    char memoryInfo[100];

    int getCpuUse() const;

Q_SIGNALS:

protected:

    virtual void run();

};

#endif // QSYSTEMINFO_H
