#include "mesmqttclient.h"
#include "../appconfig.h"

#include "../tools/MQTTClient.h"
#include "../dao/mesproducteds.h"
#include "mesmsghandler.h"

//d2s/设备序列号/协议版本/data	下位机上传数据
//s2d/设备序列号/协议版本/data	上位机下传数据
//S2d/设备序列号/协议版本/config	配置消息下发

volatile MQTTClient_deliveryToken deliveredtoken;

#define QOS 1

//
static void delivered(void *context, MQTTClient_deliveryToken dt)
{
    qDebug()<< QString("Message with token value %1 delivery confirmed").arg((int)dt);
    deliveredtoken = dt;
}

//MQTT API:收到消息后的回调,我订阅的主题的消息都到这里来
static int _msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    //int i;
    //char* payloadptr;
    //变成QT形式
    QString topic(topicName);
    QByteArray msg;
    msg.append((char*)message->payload,message->payloadlen);

    MESMqttClient *client=(MESMqttClient*)context;
    //让QT类处理c->c++
    client->msgarrvd(topic,msg);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    qDebug()<< "\nConnection lost\n";
    qDebug()<< QString(" cause: %1").arg(cause);
}

MESMqttClient *MESMqttClient::self;

MESMqttClient::MESMqttClient(QObject *parent) :
    QThread(parent)
{
    self=this;
    mqtClient=NULL;
    connectFlag=false;
#if QT_VERSION < 0x050000
    interruptionRequested=false;
#endif
}

MESMqttClient *MESMqttClient::getInstance()
{
    return self;
}

bool MESMqttClient::sendMsg(QString &topic, QByteArray &msg)
{
    QByteArray ba = topic.toLatin1();
    const char *c_topic = ba.data();
    int length=msg.length();
    int rc=0;
    bool ret=false;

#define TIMEOUT     10000L

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    mutex.lock();
    if(mqtClient!=NULL){
        pubmsg.payload = msg.data();
        pubmsg.payloadlen = length;
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(mqtClient, c_topic, &pubmsg, &token);
        rc = MQTTClient_waitForCompletion(mqtClient, token, TIMEOUT);
        if(rc==MQTTCLIENT_SUCCESS)
            ret=true;
    }
    mutex.unlock();
    return ret;
}

bool MESMqttClient::sendD2S(QByteArray &msg)
{
    QString topic = QString("d2s/%1/2/data").arg(AppConfig::getInstance().DeviceSN);
    return sendMsg(topic,msg);
}

bool MESMqttClient::sendD2S(quint8 tag, quint16 key, QString &msg)
{
    MESPacket packet;
    packet.versions=2;
    packet.tag=tag;
    packet.body.key=key;
    packet.body.setData(msg);
    QByteArray packetBytes=packet.toByteArray();
    qDebug() << QString("tag:%1,Key:%2").arg(packet.tag).arg(packet.body.key);
    return sendD2S(packetBytes);
}

#include "../dao/mesdispatchorder.h"

//Mqtt收到的消息,MQTT的回调过来
void MESMqttClient::msgarrvd(QString &topic, QByteArray &msg)
{
    MESPacket packet;
    MESPacket::toMESPacket(msg,packet);

    qDebug() << QString("收到 %1,TAG:%2,KEY:%3").arg(topic).arg(packet.tag).arg(packet.body.key);
    if(topic=="s2d/444/2/order"){
        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(msg, &json_error);

        if(json_error.error == QJsonParseError::NoError) {
            QJsonObject obj=parse_doucment.object();
            MESDispatchOrder::onSubimtOrder(obj);
        }
        return;
    }
    //把收到的主题和消息,仍给处理任务
    MESMsgHandler::getInstance()->appendMsg(topic,packet);
}

void MESMqttClient::restartConnect()
{
    closeConnect();
}

void MESMqttClient::closeConnect()
{
    mutex.lock();
    if(mqtClient!=NULL){
        MQTTClient_disconnect(mqtClient, 10000);
    }
    mutex.unlock();
}

bool MESMqttClient::isConnected()
{
    return connectFlag;
}

#if QT_VERSION < 0x050000
void MESMqttClient::requestInterruption()
{
    QMutexLocker locker(&selfMutex);
    interruptionRequested=true;
}

bool MESMqttClient::isInterruptionRequested()
{
    QMutexLocker locker(&selfMutex);
    return interruptionRequested;
}
#endif

#include "signal.h"

//线程任务
void MESMqttClient::run()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    MQTTClient_SSLOptions sslopts = MQTTClient_SSLOptions_initializer;
    QString topic;
    QString topicWill;
    QString topicTest;
    int onLineId=0;
    int rc=0;

    signal(SIGPIPE,SIG_IGN);

    statusMessage="开始运行";

    while (!isInterruptionRequested())
    {
        QUuid uuid=QUuid::createUuid();
        QString clinetID=uuid.toString();
        QByteArray curl = url.toLatin1();
        QByteArray cid = clinetID.toLatin1();
        QByteArray cuser= userName.toLatin1();
        QByteArray cpass = password.toLatin1();
        QByteArray cwillMess;
        QByteArray ctopicWill;
        QJsonObject willMsgJson;

        topic = QString("s2d/%1/2/data").arg(AppConfig::getInstance().DeviceSN);
        topicTest = QString("s2d/%1/2/order").arg(AppConfig::getInstance().DeviceSN);
        topicWill= QString("will/%1").arg(AppConfig::getInstance().DeviceSN);
        ctopicWill=topicWill.toLatin1();

        onLineId=QDateTime::currentMSecsSinceEpoch()%10000;

        connectFlag=false;
        qDebug() << QString("connect : %1,clientId:%2").arg(url).arg(clinetID);
        if(url.length()==0)
        {
            QThread::sleep(1);
            continue;
        }

        willMsgJson.insert("sn",AppConfig::getInstance().DeviceSN);
        willMsgJson.insert("onLineId",onLineId);
        cwillMess=QJsonDocument(willMsgJson).toJson();

        mutex.lock();

        MQTTClient_create(&mqtClient, curl.data(), cid.data(),
                          MQTTCLIENT_PERSISTENCE_NONE, this);

        conn_opts.keepAliveInterval = 20; //保持，心跳
        conn_opts.cleansession = 1;

        conn_opts.will = &will_opts;
        conn_opts.will->message = cwillMess.data();
        conn_opts.will->qos = 1;
        conn_opts.will->topicName = ctopicWill.data();
        conn_opts.will->retained = 0;

        if(cuser.length()>0)
            conn_opts.username=cuser.data(); //用户名
        if(cpass.length()>0)
            conn_opts.password=cpass.data(); //密码

        //设定消息接收回调
        MQTTClient_setCallbacks(mqtClient, NULL, connlost, _msgarrvd, delivered);

        mutex.unlock();
        connectFlag=false;
        //连接
        statusMessage="开始连接...";
        if ((rc = MQTTClient_connect(mqtClient, &conn_opts)) != MQTTCLIENT_SUCCESS) {
            qDebug()<< QString("Failed to connect, return code %1").arg(rc);
            statusMessage="正在重新连接...";
            //restart....
            QThread::sleep(1);

            mutex.lock();
            MQTTClient_disconnect(mqtClient, 10000);
            MQTTClient_destroy(&mqtClient);
            mqtClient=NULL;
            mutex.unlock();
            continue;
        }

        connectFlag=true;
        //订阅主题
        mutex.lock();
        MQTTClient_subscribe(mqtClient,topic.toLatin1().data(),1);
        MQTTClient_subscribe(mqtClient,topicTest.toLatin1().data(),1);
        mutex.unlock();
        qDebug()<< QString("subscribe Topic: %1").arg(topic);
        qDebug()<< "subscribe Topic:" << topicTest;

        statusMessage="连接成功";

        //send online topic
        {
            QString onlineTopic=QString("online/%1").arg(AppConfig::getInstance().DeviceSN);
            QJsonObject jsonOnline;
            jsonOnline.insert("sn",AppConfig::getInstance().DeviceSN);
            jsonOnline.insert("onLineId",onLineId);
            QByteArray msg=QJsonDocument(jsonOnline).toJson();
            sendMsg(onlineTopic, msg);
        }

        while (!isInterruptionRequested()  && 1==MQTTClient_isConnected(mqtClient) ) {

            //subimt
            MESProducteds  producteds;
            if(MESProducteds::onCountMsg()>0){
                if( MESProducteds::onFirstMsg(producteds) ){
                    //subimt......
                    //qDebug() << QString("生产数据:%1,%2 %3").arg(producteds.CreatedTime).arg(producteds.TagType).arg(producteds.Msg);
                    //生产数据上传
                    qDebug() <<"生产数据上传";
                    this->sendD2S(producteds.TagType,0x01,producteds.Msg);
                    MESProducteds::onDelete(producteds.ID);
                }
            }
            QThread::sleep(1);
        }

        mutex.lock();
        MQTTClient_disconnect(mqtClient, 10000);
        MQTTClient_destroy(&mqtClient);
        mqtClient=NULL;
        mutex.unlock();

        statusMessage="连接断开";
        connectFlag=false;
        qDebug() << "restart connect .................";
    }
    statusMessage="退出运行";
    qDebug() << "Thread exit**************";
}
