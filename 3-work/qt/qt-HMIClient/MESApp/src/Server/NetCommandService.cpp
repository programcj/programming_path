/*
 * NetCommandService.cpp
 *
 *  Created on: 2015年4月24日
 *      Author: cj
 */

#include "NetCommandService.h"

NetCommandService *NetCommandService::Instance;

NetCommandService::NetCommandService(QObject* parent) :
    QObject(parent)
{
    this->flagStart=false;
    Instance=this;
    this->tcpServer = new QTcpServer(this);
    //监听是否有客户端来访，且对任何来访者监听，端口为6666
    udpSocket = new QUdpSocket(this); //创建一个QUdpSocket类对象，该类提供了Udp的许多相关操作

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    //readyRead()信号是每当有新的数据来临时就被触发
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(udpRead()));
}

NetCommandService::~NetCommandService()
{
    delete udpSocket;
    delete tcpServer;
}

NetCommandService *NetCommandService::GetInstance()
{
    return Instance;
}

bool NetCommandService::isStart()
{
    return this->flagStart;
}

bool NetCommandService::start()
{
    this->flagStart=false;
    //此处的bind是个重载函数，连接本机的port端口，
    //采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是
    //用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    if(! udpSocket->bind(6667,
                         QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint) )
    {
        logDebug(udpSocket->errorString());
        return false;
    }
    if (!tcpServer->listen(QHostAddress::Any, 6666))
    {
        logDebug(tcpServer->errorString());
        return false;
    }
    this->flagStart=true;
    return true;
}

bool NetCommandService::close()
{
    this->flagStart=false;
    udpSocket->close();
    tcpServer->close();
    return true;
}

void NetCommandService::acceptConnection()
{
    tcpSocket = tcpServer->nextPendingConnection();

    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(revData()));
    //当tcpSocket在接受客户端连接时出现错误时，displayError(QAbstractSocket::SocketError)进行错误提醒并关闭tcpSocket。

    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}

void NetCommandService::revData()
{
    QString datas = tcpSocket->readAll();
    QString retStr = "";
    //接收字符串数据。
    if (datas.startsWith("SQL:"))
    {
        datas.remove(0, 4);
        QSqlQuery query(SQLiteBaseHelper::getInstance().getDB());
        if (query.exec(datas))
        {
            int lineSize = query.size();
            int columnSize = query.record().count();

            retStr.sprintf("result:line[%d]column[%d]\r\n", lineSize,
                           columnSize);

            for (int i = 0; i < columnSize; i++)
            {
                retStr += query.record().fieldName(i);
                retStr += "|";
            }
            retStr += "\r\n";

            while (query.next())
            {
                for (int i = 0; i < columnSize; i++)
                {
                    retStr += query.value(i).toString();
                    retStr += "|";
                }
                retStr += "\r\n";
            }
        }
        else
        {
            retStr = "err:" + query.lastError().text();
        }
    }
    if (datas.startsWith("SQLM:"))
    {
        datas.remove(0, 5);
        QSqlQuery query(SQLiteProductedHelper::getInstance().getDB());
        if (query.exec(datas))
        {
            int lineSize = query.size();
            int columnSize = query.record().count();

            retStr.sprintf("result:line[%d]column[%d]\r\n", lineSize,
                           columnSize);

            for (int i = 0; i < columnSize; i++)
            {
                retStr += query.record().fieldName(i);
                retStr += "|";
            }
            retStr += "\r\n";

            while (query.next())
            {
                for (int i = 0; i < columnSize; i++)
                {
                    retStr += query.value(i).toString();
                    retStr += "|";
                }
                retStr += "\r\n";
            }
        }
        else
        {
            retStr = "err:" + query.lastError().text();
        }
    }
    retStr += "\r\n";
    tcpSocket->write(retStr.toAscii());
    tcpSocket->close();
}

void NetCommandService::udpRead()
{
    while (udpSocket->hasPendingDatagrams())
    {
        QHostAddress hostAddr;
        quint16 senderPort;
        QByteArray datagram;
        //pendingDatagramSize为返回第一个在等待读取报文的size，
        //resize函数是把datagram的size归一化到参数size的大小一样
        datagram.resize(udpSocket->pendingDatagramSize());
        //将读取到的不大于datagram.size()大小数据输入到datagram.data()中，datagram.data()返回的是一个字节数组中存储
        //数据位置的指针
        udpSocket->readDatagram(datagram.data(), datagram.size(), &hostAddr,
                                &senderPort);
        //logDebug(QString("UDP[%1:%2]:%3").arg(hostAddr.toString()).arg(senderPort).arg(datagram.data()));
        QString msg = "6666,posrt is start.my is mes.";
        udpSocket->writeDatagram(msg.toAscii().data(), msg.toAscii().size(),
                                 hostAddr, senderPort); //QHostAddress::Broadcast
    }
}

void NetCommandService::displayError(QAbstractSocket::SocketError socketError)
{
    qDebug() << tcpSocket->errorString();
    tcpSocket->close();
}
