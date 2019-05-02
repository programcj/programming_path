#include <QNetworkInterface>
#include "netbroadhandler.h"
#include "../dao/mesdispatchorder.h"
#include "../appconfig.h"
#include "../tools/mestools.h"

#define PORT_SERVER 49999

NetBroadHandler::NetBroadHandler(QObject *parent) : QThread(parent)
{
    udpSocket= new QUdpSocket(this);
    if(udpSocket->bind(PORT_SERVER)){ //qDebug() << "bind success";
        connect(udpSocket, SIGNAL(readyRead()),this, SLOT(updReadyRead()));
    }
}

NetBroadHandler::~NetBroadHandler()
{
    udpSocket->close();
}

void NetBroadHandler::updReadyRead()
{
    qDebug() << "read...";
    QHostAddress host;
    quint16 port;
    while(udpSocket->hasPendingDatagrams()){
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(),&host,&port);

        QJsonParseError err;
        QJsonDocument document=QJsonDocument::fromJson(datagram,&err);
        if(err.error== QJsonParseError::NoError) {
            QJsonObject jsonObj=document.object();

            if(jsonObj["ipInfo"].toInt()==1)
            {
                QJsonObject interJson=myNetworkInterfaceInfo();
                QByteArray byteData=MESTools::jsonToQString(interJson).toLatin1();
                udpSocket->writeDatagram(byteData,host,port);
                if(jsonObj["toBroadcast"].toInt()==1){
                    udpSocket->writeDatagram(byteData,QHostAddress::Broadcast,port);
                }
            }
            if(jsonObj["showSN"].toInt()==1)
            {
                QJsonArray array;
                QJsonObject echoJson;
                MESDispatchOrder::onGetDispatchNoBaseInfos(array);
                echoJson.insert("SN",AppConfig::getInstance().DeviceSN);
                echoJson.insert("DispatchNoArray",array);
                QByteArray byteData=QJsonDocument(echoJson).toJson();

                udpSocket->writeDatagram(byteData,host,port);
                if(jsonObj["toBroadcast"].toInt()==1){
                    udpSocket->writeDatagram(byteData,QHostAddress::Broadcast,port);
                }
            }
            if(jsonObj["setip"]==1){

            }
        }
    }
}

QJsonObject NetBroadHandler::myNetworkInterfaceInfo()
{
    QJsonObject interJson;

    //本机IP列表
    QList<QNetworkInterface> interfacelist=QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface interface, interfacelist)
    {
        QJsonObject obj;
        QJsonArray ipArray;
        //obj.insert("device",interface.name()); //设备名
        obj.insert("macAddress:", interface.hardwareAddress());//硬件地址
        //获取IP地址条目列表，每个条目中包含的IP地址，子网掩码，广播地址
        QList<QNetworkAddressEntry> entrylist = interface.addressEntries();
        foreach(QNetworkAddressEntry entry, entrylist)
        {
            QJsonObject ipobj;
            ipobj.insert("ip",entry.ip().toString());//ip地址
            //ipobj.insert("netmask",entry.netmask().toString());//子网掩码
            //ipobj.insert("broadcast",entry.broadcast().toString());//广播地址
            ipArray.append(ipobj);
        }
        obj.insert("address",ipArray);
        interJson.insert(interface.name(),obj);
    }
    return interJson;
}

void NetBroadHandler::run()
{
    QUdpSocket socket;

    socket.bind(PORT_SERVER+1);
    {
//        QList<QHostAddress> ipList = QNetworkInterface::allAddresses();
//        for (int i = 0; i < ipList.size(); i++)
//        {
//            qDebug()<< ipList[i].toString();
//        }
    }
    QJsonObject echoJson;

    echoJson.insert("showSN",1);
    echoJson.insert("ipInfo",1);
    echoJson.insert("toBroadcast",1);

    QByteArray array=QJsonDocument(echoJson).toJson();

    //    （1）公认端口（WellKnownPorts）：从0到1023，
    //    （2）注册端口（RegisteredPorts）：从1024到49151
    //    （3）动态和/或私有端口（Dynamicand/orPrivatePorts）：
    //从49152到65535。理论上，不应为服务分配这些端口。
    //实际上，机器通常从1024起分配动态端口。但也有例外：SUN的RPC端口从32768开始。

    int wlen=socket.writeDatagram(array,array.length(),QHostAddress::Broadcast,PORT_SERVER);
    qDebug() << "Send Len:"<<wlen;
    QThread::sleep(1);
    socket.writeDatagram(array,array.length(),QHostAddress::Broadcast,PORT_SERVER);
    QThread::sleep(1);
    socket.writeDatagram(array,array.length(),QHostAddress::Broadcast,PORT_SERVER);
    QThread::sleep(1);
    socket.writeDatagram(array,array.length(),QHostAddress::Broadcast,PORT_SERVER);
    qDebug()<<"send data success";
    exec();
}
