#ifndef HALRFIDREAD_H
#define HALRFIDREAD_H

#include <QThread>
#include <QMutexLocker>
#include <QDebug>
#include "../public.h"

/**
  *读卡线程
 * @brief The HALRFIDRead class
 */
class HALRFIDRead : public QThread
{
#if QT_VERSION < 0x050000
    QMutex selfMutex;
    bool interruptionRequested;
#endif

    Q_OBJECT
public:
    explicit HALRFIDRead(QObject *parent = 0);
    ~HALRFIDRead();

#if QT_VERSION < 0x050000
    void requestInterruption();
    bool isInterruptionRequested();
#endif

signals:
    void sig_ReadRFIDUID(QString uid);

public slots:
    virtual void run() ; //qt5 Q_DECL_OVERRIDE
};

#endif // HALRFIDREAD_H
