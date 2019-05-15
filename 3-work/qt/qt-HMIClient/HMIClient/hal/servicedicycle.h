#ifndef SERVICEDICYCLE_H
#define SERVICEDICYCLE_H

#include <QThread>
#include <QDateTime>
#include "../public.h"
#include <QMutexLocker>


/**
 * DI采集计算服务
 *
 * @brief The ServiceDICycle class
 */
class ServiceDICycle: public QThread
{
    // ScanConfig-GPIO
    QString scanModule;

    int lineA[4];
    int lineB[4];
    int lineC[4];

    static ServiceDICycle *self;

#if QT_VERSION==0x040805
    QMutex selfMutex;
    bool interruptionRequested;
#endif

    Q_OBJECT
public:
    static void saveConfigInfo(QJsonObject &json);

    explicit ServiceDICycle(QObject *parent = 0);

    static ServiceDICycle *getInstance();

    void loadConfigInfo();

#if QT_VERSION < 0x050000
    void requestInterruption();
    bool isInterruptionRequested();
#endif

Q_SIGNALS:
    //模次, 成型周期
    void sigTotalNumAdd(int TotalNum,int endmsc);

private:
    void onCountAdd(QDateTime &lineAstart, QDateTime &lineAnextStart, QDateTime &lineBstart, QDateTime &lineBend, QDateTime &lineCend);

    void readLine(int line, int *value);

public slots:
    virtual void run() ; //Q_DECL_OVERRIDE
};

#endif // SERVICEDICYCLE_H
