#ifndef NETBROADHANDLER_H
#define NETBROADHANDLER_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include "../public.h"

class NetBroadHandler : public QThread
{
    QUdpSocket *udpSocket;

    Q_OBJECT
public:
    explicit NetBroadHandler(QObject *parent = 0);
    ~NetBroadHandler();

signals:

public slots:
    void updReadyRead();

protected:
    QJsonObject myNetworkInterfaceInfo();

    virtual void run();
};

#endif // NETBROADHANDLER_H
