#include "mesmsghandler.h"
#include "../hal/servicedicycle.h"

#include "../dao/mesdispatchorder.h"
#include "../dao/sqlitebasehelper.h"
#include "mesmqttclient.h"
#include "../tools/mestools.h"

#define RESPONSE_WAIT_TIME (1000*6)

MESMsgHandler *MESMsgHandler::self=NULL;

MESMsgHandler::MESMsgHandler(QObject *parent) :
    QThread(parent)
{
    self=this;
#if QT_VERSION < 0x050000
    interruptionRequested=false;
#endif
    myCurkey=0;
}

MESMsgHandler *MESMsgHandler::getInstance()
{
    return self;
}

#if QT_VERSION < 0x050000
void MESMsgHandler::requestInterruption()
{
    QMutexLocker locker(&selfMutex);
    interruptionRequested=true;
}

bool MESMsgHandler::isInterruptionRequested()
{
    QMutexLocker locker(&selfMutex);
    return interruptionRequested;
}
#endif

void MESMsgHandler::requestInterruption2()
{
    requestInterruption();

    mutexW.lock();
    waitAppMsg.wakeAll(); //唤醒等待
    mutexW.unlock();
}

bool MESMsgHandler::isConnected()
{
    if(MESMqttClient::getInstance()!=NULL){
        return MESMqttClient::getInstance()->isConnected();
    }
    return false;
}

int MESMsgHandler::appendMsg(const QString &topic, const MESPacket &packet)
{
    MESMsg msg;
    msg.topic=topic;
    msg.packet=packet;
    //
    qDebug() << QString("Append msg %1").arg(topic);
    //
    mutex.lock();
    msgList.append(msg);
    mutex.unlock();

    //
    mutexW.lock();
    waitAppMsg.wakeAll(); //唤醒等待
    mutexW.unlock();

    return msgList.length();
}

//0x12 刷卡数据 上发数据 刷卡产生的上传数据
bool MESMsgHandler::submitBrushCard(QJsonObject &json, QString &msg)
{
    QString submitMsg=QString(QJsonDocument(json).toJson());
    QString str=MESTools::jsonToQString(json);
    qDebug() << "上发-刷卡数据:" << str;
    bool ret=false;
    MESMsgResponse response;

    response.responseFlag=false;
    response.tag=0x12;

    responseMutex.lock();
    response.key=myCurkey++;
    responseList.append(&response);
    responseMutex.unlock();

    if(MESMqttClient::getInstance()->sendD2S(response.tag, response.key, submitMsg)){
        response.mutex.lock();
        response.wait.wait(&response.mutex,RESPONSE_WAIT_TIME); ///not
        response.mutex.unlock();
        if(response.responseFlag){
            msg=response.msg;
            if(response.result==0)
                ret=true;
        } else {
            msg="数据提交出错，服务器未响应";
        }
    } else {
        msg="数据提交出错，请检查网络!";
    }

    responseMutex.lock();
    responseList.removeOne(&response);
    responseMutex.unlock();
    return ret;
}

//0x13 次品数据 上发数据 生产次品数
bool MESMsgHandler::submitBadData(QJsonObject &json, QString &msg)
{
    QString submitMsg=QString(QJsonDocument(json).toJson());
    QString str=MESTools::jsonToQString(json);
    qDebug() << "上发-次品数据:" << str;

    bool ret=false;
    MESMsgResponse response;

    response.responseFlag=false;
    response.tag=0x13;

    responseMutex.lock();
    response.key=myCurkey++;
    responseList.append(&response);
    responseMutex.unlock();

    if(MESMqttClient::getInstance()->sendD2S(response.tag, response.key, submitMsg)){
        response.mutex.lock();
        response.wait.wait(&response.mutex,RESPONSE_WAIT_TIME);
        response.mutex.unlock();

        if(response.responseFlag){
            msg=response.msg;
            if(response.result==0)
                ret=true;
        } else {
            msg="数据提交出错，服务器未响应";
        }
    } else {
        msg="数据提交出错，请检查网络!";
    }

    responseMutex.lock();
    responseList.removeOne(&response);
    responseMutex.unlock();

    return ret;
}

//0x14 报警数据 上发数据 下位机相关报警
bool MESMsgHandler::submitAlarmData(QJsonObject &json, QString &msg)
{
    QString submitMsg=QString(QJsonDocument(json).toJson());
    QString str=MESTools::jsonToQString(json);
    qDebug() << "上发-报警数据:" << str;

    bool ret=false;
    MESMsgResponse response;

    response.responseFlag=false;
    response.tag=0x14;

    responseMutex.lock();
    response.key=myCurkey++;
    responseList.append(&response);
    responseMutex.unlock();

    if(MESMqttClient::getInstance()->sendD2S(response.tag, response.key, submitMsg)){
        response.mutex.lock();
        response.wait.wait(&response.mutex,RESPONSE_WAIT_TIME);
        response.mutex.unlock();
        if(response.responseFlag){
            msg=response.msg;
            if(response.result==0)
                ret=true;
        } else {
            msg="数据提交出错，服务器未响应";
        }
    } else {
        msg="数据提交出错，请检查网络!";
    }

    responseMutex.lock();
    responseList.removeOne(&response);
    responseMutex.unlock();
    return ret;
}

//任务运行中的，核心
void MESMsgHandler::run()
{
    MESMsg msg;
    qDebug() << "handler run..";

    while(!isInterruptionRequested()){
        //等待
        mutexW.lock();
        waitAppMsg.wait(&mutexW,500);  //ms not us
        mutexW.unlock();

        /**
          * 遍历List，消息队列,处理给我的主题 的消息
          */
        while(!isInterruptionRequested()){
            mutex.lock();
            if(msgList.length()==0)
            {
                mutex.unlock();
                break;
            }
            msg=msgList[0];

            msgList.removeFirst();
            mutex.unlock();

            MESPacketBody &body=msg.packet.body;
            QJsonParseError json_error;
            QJsonDocument parse_doucment = QJsonDocument::fromJson(body.data, &json_error);

            qDebug() << QString("%1,%2").arg(msg.topic).arg(QString(body.data)).arg(msg.packet.tag);

            if(json_error.error == QJsonParseError::NoError) {
                QJsonObject obj=parse_doucment.object();

                switch(msg.packet.tag){ //包头里面的VTL部分判断... TAG判断
                case 0x11:// 生产数据
                case 0x12:// 刷卡数据
                case 0x13:// 次品数据
                case 0x14:// 报警数据
                    downData_echo(msg.packet.tag, msg.packet.body.key, msg.packet.body.data);
                    break;
                case 0x21://	机器派单	下发数据	s2d/设备序列号/协议版本/data	服务端派工单
                {
                    downData_0x21(msg,body,obj);
                    break;
                }
                case 0x22://	设备控制	下发数据	s2d/设备序列号/协议版本/data	服务端下发设备控制指令
                {
                    downData_0x22(msg,body,obj);
                    break;
                }
                }
            } else {
                qDebug() << "json format err...";
            }

        }
    }

    qDebug() << "Thread exit**************";
}

/**
 * 机器派单	下发数据	s2d/设备序列号/协议版本/data	服务端派工单
 * @brief MESMsgHandler::downData_0x21
 * @param body
 * @param obj
 */
void MESMsgHandler::downData_0x21(MESMsg &msg,MESPacketBody &body, QJsonObject &obj)
{
    //如果 派工单号 PRIMARY KEY [CSCP] 存在，怎么返回？
    QString MO_DispatchNo=obj["MO_DispatchNo"].toString();

    if (MESDispatchOrder::onCountPK(MO_DispatchNo) != 0){
        //此派工单已经存在
        QJsonObject resJson;
        resJson.insert("result",-1);
        // resJson.insert("msg","then dispathch no exists");
        ////////////////////////////////////////////////////////////////////////
        //        MESPacket packet;
        //        packet.versions=msg.packet.versions;
        //        packet.tag=0x21;
        //       MESPacketBody body=MESPacketBody(body.key, resJson);
        //        packet.setBody(body);
        //        QByteArray packetBytes=packet.toByteArray();
        /////////////////////////////////////////////////////////////////////////
        QString responseMsg=QJsonDocument(resJson).toJson();
        MESMqttClient::getInstance()->sendD2S(0x21,body.key,responseMsg);
    } else {
        MESDispatchOrder::onSubimtOrder(obj);
    }

    ///////////
    //回复结果
}

/**
 * 设备控制	下发数据	s2d/设备序列号/协议版本/data	服务端下发设备控制指令
 * @brief MESMsgHandler::downData_0x22
 * @param body
 * @param obj
 */
void MESMsgHandler::downData_0x22(MESMsg &msg,MESPacketBody &body, QJsonObject &obj)
{
    int FDAT_CommandType=obj.take("FDAT_CommandType").toInt();
    switch(FDAT_CommandType){
    case 0x01://：HMI关机
    case 0x02://：HMI重启
    case 0x03://: HMI 升级
    case 0x04://自定义
        break;
    }
}

/**
 * 0x31	配置消息	下发数据（参数配置） S2d/设备序列号/协议版本/config
 * @brief MESMsgHandler::downData_0x31
 * @param msg
 * @param body
 * @param obj
 */
void MESMsgHandler::downData_0x31(MESMsg &msg, MESPacketBody &body, QJsonObject &obj)
{
    int FDAT_ConfigType=obj["FDAT_ConfigType"].toInt();
    QJsonObject FDAT_ConfigDataJson=obj["FDAT_ConfigData"].toObject();
    if(FDAT_ConfigType==1){
        ServiceDICycle::saveConfigInfo(FDAT_ConfigDataJson);
    }
}

void MESMsgHandler::downData_echo(int tag, int key, QByteArray &msgBody)
{
    MESMsgResponse *presponse;
    QJsonParseError json_error;
    QJsonDocument parseJson = QJsonDocument::fromJson(msgBody, &json_error);
    int result=-1;
    QString msg="json format error";

    if(json_error.error == QJsonParseError::NoError) {
        QJsonObject json=parseJson.object();
        if(json.contains("result")){
            result=json["result"].toInt();
            msg=json["msg"].toString();
        }
    } else {
        msg="Json format error";
    }

    responseMutex.lock();
    for(int i=0,j=responseList.length();i<j;i++){
        presponse=responseList[i];
        if(presponse->key==key && presponse->tag==tag && presponse->responseFlag==false){
            presponse->msg=msg;
            presponse->result=result;
            presponse->responseFlag=true;

            presponse->mutex.lock();
            presponse->wait.wakeAll(); //唤醒等待
            presponse->mutex.unlock();
        }
    }
    responseMutex.unlock();
}
