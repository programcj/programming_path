#ifndef MESMQTTCLIENT_H
#define MESMQTTCLIENT_H

#include <QByteArray>
#include <QObject>
#include <QThread>
#include <QDebug>
#include <QList>
#include <QMutex>
#include <QSemaphore>
#include <QUuid>

#include "mespacket.h"

///MQTT主题
//d2s/设备序列号/协议版本/data	下位机上传数据
//s2d/设备序列号/协议版本/data	上位机下传数据
//S2d/设备序列号/协议版本/config	配置消息下发

class MESMqttClient : public QThread
{
    QMutex mutex;

    static MESMqttClient *self;

#if QT_VERSION < 0x050000
    QMutex selfMutex;
    bool interruptionRequested;
#endif

    void *mqtClient;

    QString statusMessage;

    Q_OBJECT
public:
    QString url;
    QString userName;
    QString password;

    explicit MESMqttClient(QObject *parent=0);

    static MESMqttClient *getInstance();

    //发送消息,TOPIC主题,Msg内容
    bool sendMsg(QString &topic,QByteArray &msg);

    //
    bool sendD2S(QByteArray &msg);

    bool sendD2S(quint8 tag,quint16 key,QString &msg);

    //bool sendD2SWaitResponse(quint8 tag, quint16 key,QString &msg);

    //Mqtt收到的消息,MQTT的回调过来
    void msgarrvd(QString &topic,QByteArray &msg);

    //重连
    void restartConnect();

    void closeConnect();

    bool isConnected();

#if QT_VERSION < 0x050000
    void requestInterruption();
    bool isInterruptionRequested();
#endif

    QString getStatusMsg(){
        return statusMessage;
    }

private:
    bool connectFlag;

    //QT的线程运行start后，自动到这里
    virtual void run();
};

#endif // MESMQTTCLIENT_H
