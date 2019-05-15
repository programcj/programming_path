#ifndef MESMSGHANDLER_H
#define MESMSGHANDLER_H

#include <QtCore>
#include <QList>

#include <QWaitCondition>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QMutexLocker>
#include "../public.h"

#include "mespacket.h"
#include "mesmsghandler.h"

class MESMsg {
public:
    QString topic;
    MESPacket packet;
};

class MESMsgResponse {
public:
    bool responseFlag;
    int tag;
    int key;
    //QByteArray packet; //response
    QString msg;
    int result;

    QWaitCondition wait; //等待信号
    QMutex mutex; //等待信号专用

    MESMsgResponse(){
        responseFlag=false;
        tag=0;
        key=0;
        result=-1;
    }
};

//把收到的主题和消息,仍给处理任务
class MESMsgHandler: public QThread
{
    Q_OBJECT

#if QT_VERSION < 0x050000
    QMutex selfMutex;
    bool interruptionRequested;
#endif

    QList<MESMsg> msgList; //收到消息队列
    QMutex mutex; //队列锁

    QWaitCondition waitAppMsg; //等待信号
    QMutex mutexW; //等待信号专用

    QList<MESMsgResponse*> responseList;
    QMutex responseMutex; //队列锁
    quint16 myCurkey;
    static MESMsgHandler *self;

public:
    explicit MESMsgHandler(QObject *parent=0);
    static MESMsgHandler *getInstance();

#if QT_VERSION < 0x050000
    void requestInterruption();
    bool isInterruptionRequested();
#endif

    void requestInterruption2();

    bool isConnected();

    //MQTT收到消息到这里
    int appendMsg(const QString &topic,const MESPacket &packet);

    //0x12 刷卡数据 上发数据 刷卡产生的上传数据
    bool submitBrushCard(QJsonObject &json, QString &msg);

    //0x13 次品数据 上发数据 生产次品数
    bool submitBadData(QJsonObject &json, QString &msg);

    //0x14 报警数据 上发数据 下位机相关报警
    bool submitAlarmData(QJsonObject &json, QString &msg);

    bool submitProducteds();
    //信号与槽的，收到工单后，发送信号，UI可以显示...
Q_SIGNALS:
    void sig_DownOrder(QString &SendNo,int Type);

private:
    virtual void run() ; //任务运行中的，核心

    //0x21	机器派单	下发数据	s2d/设备序列号/协议版本/data	服务端派工单
    //0x22	设备控制	下发数据	s2d/设备序列号/协议版本/data	服务端下发设备控制指令
    //0x31	配置消息	下发数据（参数配置）	S2d/设备序列号/协议版本/config	系统（本地化）参数（计数信号和正负逻辑等。
    void downData_0x21(MESMsg &msg,MESPacketBody &body, QJsonObject &obj);
    void downData_0x22(MESMsg &msg,MESPacketBody &body, QJsonObject &obj);
    void downData_0x31(MESMsg &msg,MESPacketBody &body, QJsonObject &obj);
    void downData_echo(int tag,int key, QByteArray &msgBody);
    //void downLoadToOrder(QJsonObject &obj);
};

#endif // MESMSGHANDLER_H
