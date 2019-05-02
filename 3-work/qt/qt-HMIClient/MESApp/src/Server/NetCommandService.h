/*
 * NetCommandService.h
 * 通过网络命令进行一些操作,如查看数据库什么的
 *  Created on: 2015年4月24日
 *      Author: cj
 */

#ifndef NETCOMMANDSERVICE_H_
#define NETCOMMANDSERVICE_H_

#include <QtCore>
#include <QtNetwork>
#include "../Public/public.h"

/*
 * 通过网络命令进行一些操作,如查看数据库什么的,也可查看系统变量等
 */
class NetCommandService: public QObject
{
Q_OBJECT

	QTcpServer *tcpServer;
	QTcpSocket *tcpSocket;
	QUdpSocket *udpSocket;
    bool flagStart;
    static NetCommandService *Instance;
public:
	explicit NetCommandService(QObject *parent = 0);
	~NetCommandService();
    static NetCommandService *GetInstance();

    bool isStart(); //是否启动
    bool start(); //启动
    bool close(); //关闭

private slots:
	void acceptConnection();
	void revData(); //接收来自服务端的数据
	void udpRead();
	void displayError(QAbstractSocket::SocketError);
};

#endif /* NETCOMMANDSERVICE_H_ */
