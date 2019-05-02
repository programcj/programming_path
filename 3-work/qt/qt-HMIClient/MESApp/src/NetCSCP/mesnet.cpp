#include "mesnet.h"
#include "mespack.h"
#include "mestool.h"
#include "mespack.h"

MESNet *MESNet::instance;

MESNet::MESNet(QObject *parent) :
		QObject(parent)
{
	d = new MESNetPrivate(this);
	instance = this;
	d->init();
}

MESNet::~MESNet()
{
	if (d != 0)
		delete d;
}
void MESNet::init(const QString &ip, qint16 portA, qint16 portB)
{
	d->serverIP = ip;
	d->portA = portA;
	d->portB = portB;
}

void MESNet::start()
{
	d->start();
}

void MESNet::close()
{
	d->close();
}

int MESNet::sendRequestADJMachine(entity::ADJMachine &v)
{
	MESADJMachineRequest request(v);
	return d->addRequestA(&request);
}

int MESNet::sendRequestFileDownStart(const QString &name, const QByteArray &ver)
{
	MESFileDownRequest request(name, ver);
	return d->addRequestB(&request);
}

int MESNet::sendRequestFileDownStream(MESFileDownStreamRequest &request)
{
	return d->addRequestB(&request);
}

int MESNet::sendRequestMESOtherRequest(MESOtherRequest &request)
{
	return d->addRequestA(&request);
}

int MESNet::sendRequestMESQualityRegulate(MESQualityRegulateRequest &request)
{
	return d->addRequestA(&request);
}

int MESNet::sendRequestMESDefectiveInfoRequest(MESDefectiveInfoRequest &request)
{
	return d->addRequestA(&request);
}

int MESNet::sendRequestMESBrushCardRequest(MESBrushCardRequest &request)
{
	return d->addRequestA(&request);
}

int MESNet::sendRequestMESProduce(MESProduceRequest &request)
{
	return d->addRequestA(&request);
}

int MESNet::sendMESOrderResponse(MESOrderResponse &response)
{
	d->addResponseB(&response);
	return 1;
}

void MESNet::addMESControlResponse(MESControlResponse &response)
{
	d->addResponseB(&response);
}

MESNet* MESNet::getInstance()
{
	return instance;
}

MESNet::State MESNet::stateA()
{
	return d->stateA;
}

MESNet::State MESNet::stateB()
{
	return d->stateB;
}

MESPack::FDATResult MESNet::getRegStatus()
{
	return d->regRequest.Result;
}

void MESNetPrivate::init()
{
	socketA = new QTcpSocket();
	socketB = new QTcpSocket();
	startFlag = false;
	QObject::connect(socketA, SIGNAL(connected()), this,
			SLOT(_q_slotConnectedA()));
	QObject::connect(socketA, SIGNAL(disconnected()), this,
			SLOT(_q_slotClosedA()));
	QObject::connect(socketA, SIGNAL(readyRead()), this,
			SLOT(_q_slotReadyReadA()));
	QObject::connect(socketA, SIGNAL(error(QAbstractSocket::SocketError)), this,
			SLOT(_q_slotErrorA(QAbstractSocket::SocketError)));
	QObject::connect(socketA, SIGNAL(bytesWritten(qint64)), this,
			SLOT(_q_slotBytesWrittenA(qint64)));

	QObject::connect(socketB, SIGNAL(connected()), this,
			SLOT(_q_slotConnectedB()));
	QObject::connect(socketB, SIGNAL(disconnected()), this,
			SLOT(_q_slotClosedB()));
	QObject::connect(socketB, SIGNAL(readyRead()), this,
			SLOT(_q_slotReadyReadB()));
	QObject::connect(socketB, SIGNAL(error(QAbstractSocket::SocketError)), this,
			SLOT(_q_slotErrorB(QAbstractSocket::SocketError)));
	QObject::connect(socketB, SIGNAL(bytesWritten(qint64)), this,
			SLOT(_q_slotBytesWrittenB(qint64)));
}

void MESNetPrivate::start()
{
	close();

	startFlag = true;
	regRequest.Result = MESPack::FDAT_RESULT_NON;

	qDebug() << "连接到服务器" << serverIP << ":" << portA;
	socketA->connectToHost(this->serverIP, this->portA);
}

void MESNetPrivate::close()
{
	startFlag = false;
	regRequest.Result = MESPack::FDAT_RESULT_NON;
	socketA->flush();
	socketA->close();
	socketA->disconnectFromHost();

	socketB->flush();
	socketB->close();
	socketB->disconnectFromHost();

	stateA = MESNet::Connecting;
	stateB = MESNet::Unconnected;
}

void MESNetPrivate::_q_slotConnectedA()
{
	qDebug() << "A连接成功时须要注册";
	stateA = MESNet::Connected;
    //请求注册包
    this->addRequestA(&regRequest);
    regTimeOut->start(2000);
}

void MESNetPrivate::_q_slotConnectedB()
{
	qDebug() << "B连接成功";
	this->stateB = MESNet::Connected;
	emit q->sigConnectBSucess();
}

void MESNetPrivate::_q_slotClosedA()
{
	stateA = MESNet::Unconnected;
	regRequest.Result = MESPack::FDAT_RESULT_FAIL_CLOSE;
	qDebug() << "通信A己断开.";
	//还要通知所有的请求
	emit q->signalReg(false, QDateTime::currentDateTime(),
			MESPack::FDAT_RESULT_FAIL_CLOSE);
	emit q->sig_protocol_produce(0xFF, MESPack::FDAT_RESULT_FAIL_CLOSE);
	emit q->sig_protocol_ic_card(0xFF, MESPack::FDAT_RESULT_FAIL_CLOSE);
	emit q->sig_protocol_defectiveInfo(MESPack::FDAT_RESULT_FAIL_CLOSE);
	emit q->sig_protocol_adj_machine(MESPack::FDAT_RESULT_FAIL_CLOSE);
	MESOtherResponse response;
	response.Result = MESPack::FDAT_RESULT_FAIL_CLOSE;
	emit q->sig_protocol_other(response);
	emit q->sig_protocol_QualityRegulate(MESPack::FDAT_RESULT_FAIL_CLOSE);
	emit q->sigCloseA();

	if (startFlag == true)
        QTimer::singleShot(1000, this, SLOT(_q_restart()));
}

void MESNetPrivate::_q_slotClosedB()
{
	//通知所有的请求 网络断开
	this->stateB = MESNet::Closing;
	MESFileDownResponse response;
	MESFileDownStreamResponse response2;
	response.Result = MESPack::FDAT_RESULT_FAIL_CLOSE;
	response2.Result = MESPack::FDAT_RESULT_FAIL_CLOSE;
	emit q->sig_MESFileDownResponse(response);
	emit q->sig_MESFileDownStreamResponse(response2);
	emit q->sigCloseB();

	//重连接B
	if (regRequest.Result == MESPack::FDAT_RESULT_REG_OK)
	{
		socketB->connectToHost(serverIP, portB);
	}
}

void MESNetPrivate::_q_slotReadyReadA()
{
	QByteArray array = socketA->readAll();
	this->buffReadA.append(array);
	//解码
	parseReadBuff(socketA, buffReadA);
}

void MESNetPrivate::_q_slotReadyReadB()
{
	QByteArray array = socketB->readAll();
	this->buffReadB.append(array);
	//解码
	parseReadBuff(socketB, buffReadB);
}

void MESNetPrivate::_q_slotErrorA(QAbstractSocket::SocketError err)
{
	qDebug() << "SOCK A 出错：_q_slotErrorA:" << err;
	switch (err)
	{
	case QTcpSocket::ConnectionRefusedError:
	case QTcpSocket::HostNotFoundError:
		break;
	case QTcpSocket::RemoteHostClosedError:
		return;
	default:
		break;
	}
	socketA->close();
	stateA = MESNet::Unconnected;
	regRequest.Result = MESPack::FDAT_RESULT_FAIL_CLOSE;
    if (startFlag == true)
        QTimer::singleShot(1000, this, SLOT(_q_restart()));
}

void MESNetPrivate::_q_slotErrorB(QAbstractSocket::SocketError err)
{
	qDebug() << "_q_slotErrorB:" << err;

	switch (err)
	{
	case QTcpSocket::ConnectionRefusedError:
	case QTcpSocket::HostNotFoundError:
		break;
	case QTcpSocket::RemoteHostClosedError:
		return;
	default:
		break;
	}
	socketB->close();
	stateB = MESNet::Unconnected;

	//重连接B
	if (regRequest.Result == MESPack::FDAT_RESULT_REG_OK)
	{
		socketB->connectToHost(serverIP, portB);
	}
}

void MESNetPrivate::_q_slotBytesWrittenA(qint64 written)
{
	qDebug() << "_q_slotBytesWrittenA:" << written;
	socketA->bytesToWrite();
}

void MESNetPrivate::_q_slotBytesWrittenB(qint64 written)
{
	qDebug() << "_q_slotBytesWrittenB:" << written;
	socketB->bytesToWrite();
}

//重新启动连接
void MESNetPrivate::_q_restart()
{
    start();
}

//注册超时
void MESNetPrivate::_slot_regTimeOut()
{
    regTimeOut->stop();
    if(regRequest.Result != MESPack::FDAT_RESULT_REG_OK)
    {
        logErr("注册超时，断开链接.");
        socketA->close();
        socketB->close();
    }
}

//解码
void MESNetPrivate::parseReadBuff(QTcpSocket *sock, QByteArray &buff)
{
	int headPosition = 0;
	int endPosition = 0;
	int bodyLength = 0;
	int frameSize = 0;

	// 找到帧头
	for (headPosition = 0; headPosition < buff.size(); headPosition++)
	{
		if (0x5A == (quint8) buff[headPosition])
			break;
	}

	if (headPosition >= buff.length() && (quint8) buff[headPosition] != 0x5A)
		return;

	if (headPosition + 4 >= buff.length()) //长度不足
		return;

	bodyLength = ((0x00FF & buff[headPosition + 4]) << 8)
			| (0x00FF & buff[headPosition + 3]);

	frameSize = 1 + 4 + bodyLength + 2;

	endPosition = headPosition + frameSize;

	if (endPosition > buff.length()) //长度不足
		return;

	MESPack pack;
	QByteArray pkBuff = buff.mid(headPosition, frameSize);
	pack.parse(pkBuff);
	//清空前段缓冲区
	buff.remove(0, endPosition);

	qDebug() << "接收包:"<<MESTool::CSCPPackToQString(pkBuff);

    if(!pack.CheckFlag())
    {
        logWarn("此数据包效验不通过。");
    }

	switch (pack.getCMDSerial())
	{
	case 0x01 + 0x80:
		protocol_regdev_parse(sock, pack); //注册响应
		break;
	case 0x10 + 0x80:
		protocol_produce_parse(sock, pack); //生产数据响应
		break;
	case 0x11 + 0x80:
		protocol_ic_card_parse(sock, pack); //ii.	发送刷卡数据 响应
		break;
	case 0x13 + 0x80:
		protocol_defectiveInfo_parse(sock, pack); //iii.	发送次品信息 响应
		break;
	case 0x14 + 0x80:
		protocol_adj_machine_parse(sock, pack); //调机 响应
		break;
	case 0x15 + 0x80:
		protocol_other_parse(sock, pack); //ii.发送其它设置信息 响应
		break;
	case 0x16 + 0x80:
		protocol_QualityRegulate_parse(sock, pack); //巡机数、打磨数 响应
		break;
	case 0x41 + 0x80:
		protocol_file_down_parse(sock, pack); //ii.	下载文件 响应
		break;
	case 0x40 + 0x80:
		protocol_file_down_parse(sock, pack); //i.	启动下载文件 响应
		break;
	case 0x17 + 0x80:
		protocol_pda_parse(sock, pack);	//PDA操作方面的 87
		//{ 0x50 + 0x80, protocol_wifi_signal_parse },
	case 0xD0:
		if (pack.getBody().length() < 5)
			protocol_wifi_signal_parse(sock, pack); //同步信号 响应 D0
		else
			protocol_order_parse(sock, pack);  //请求 派单
		break;
	case 0xD7:
		protocol_control_dev_parse(sock, pack);  //请求 控制设备
		break;
	case 0xD8:
		protocol_mail_parse(sock, pack); //ii.	下发公告
		break;
	default:
		break;
	}
}

////////////////////////////////////////////////////////////////////
//注册包响应
void MESNetPrivate::protocol_regdev_parse(QTcpSocket *socket, MESPack &pack)
{
	MESRegResponse response;

	if (!response.decode(pack))
		return;
	if (regRequest.Result == MESPack::FDAT_RESULT_REG_OK)
		return;
	//		00：终端状态正常，允许通讯
	//		01：IP地址不存在，拒绝通讯
	//		02：无效
	//		03：配置文件不完整
	if (response.echoInfo.FDAT_PMSPermit == 0x00)
	{
		//注册成功
		regRequest.Result = MESPack::FDAT_RESULT_REG_OK;
        logInfo() << "注册成功,启动端口B";
		socketB->connectToHost(this->serverIP, portB);
		emit q->signalReg(true,
				MESTool::DateTimeFormCharArray(
						response.echoInfo.FDAT_ServerSystemTime),
				MESPack::FDAT_RESULT_REG_OK);
		return;
	}
	switch (response.echoInfo.FDAT_PMSPermit)
	{
	case 0x01:
        logErr("|IP不存在");
		regRequest.Result = MESPack::FDAT_RESULT_REG_IP_NON;
		break;
	case 0x02:
        logErr("|02：无效");
		regRequest.Result = MESPack::FDAT_RESULT_REG_NON;
		break;
	case 0x03:
        logErr("|03：配置文件不完整");
		regRequest.Result = MESPack::FDAT_RESULT_REG_FILE_NON;
		break;
	default:
        logErr("注册出错");
		regRequest.Result = MESPack::FDAT_RESULT_NON;
		break;
    }
	socketA->close();
	socketA->disconnectFromHost();

	emit q->signalReg(false, QDateTime::currentDateTime(), regRequest.Result);
}
//生产数据响应
void MESNetPrivate::protocol_produce_parse(QTcpSocket *, MESPack &pack)
{
	emit q->sig_protocol_produce((quint8) pack.getBody()[0],
			FDATResultFormatAppendix(pack.getBody()[1]));
}

//ii.	发送刷卡数据 响应
void MESNetPrivate::protocol_ic_card_parse(QTcpSocket *, MESPack &pack)
{
	emit q->sig_protocol_ic_card((quint8) pack.getBody()[0],
			FDATResultFormatAppendix(pack.getBody()[1]));
}

//iii.	发送次品信息 响应
void MESNetPrivate::protocol_defectiveInfo_parse(QTcpSocket *, MESPack &pack)
{
	emit q->sig_protocol_defectiveInfo(
			FDATResultFormatAppendix(pack.getBody()[0]));
}

//调机
void MESNetPrivate::protocol_adj_machine_parse(QTcpSocket *, MESPack &pack)
{
	MESADJMachineResponse response;
	if (response.decode(pack))
	{
		emit q->sig_protocol_adj_machine(response.Result);
	}
}
//其它协议 响应
void MESNetPrivate::protocol_other_parse(QTcpSocket *, MESPack &pack)
{
	MESOtherResponse response;
	if (response.decode(pack))
		emit q->sig_protocol_other(response);
}

//巡机数、打磨数 响应
void MESNetPrivate::protocol_QualityRegulate_parse(QTcpSocket *, MESPack &pack)
{
	emit q->sig_protocol_QualityRegulate(
			FDATResultFormatAppendix(pack.getBody()[0]));
}
//文件下载响应
void MESNetPrivate::protocol_file_down_parse(QTcpSocket *sock, MESPack &pack)
{
	if (pack.getCMDSerial() == 0x40 + 0x80)
	{
		MESFileDownResponse response;
		if (response.decode(pack))
		{
			emit q->sig_MESFileDownResponse(response);
		}
	}

	if (pack.getCMDSerial() == 0x41 + 0x80)
	{
		MESFileDownStreamResponse response;
		if (response.decode(pack))
			emit q->sig_MESFileDownStreamResponse(response);
	}
}

void MESNetPrivate::protocol_pda_parse(QTcpSocket *, MESPack &)
{
}

void MESNetPrivate::protocol_wifi_signal_parse(QTcpSocket *, MESPack &pack)
{
	emit q->sig_protocol_defectiveInfo(
			FDATResultFormatAppendix(pack.getBody()[0]));
}

//请求
void MESNetPrivate::protocol_order_parse(QTcpSocket *, MESPack &pack)
{
	MESOrderRequest request;
	if (!request.decode(pack))
		return;
	emit q->sig_protocol_order(request);
}

//请求 控制设备 响应
void MESNetPrivate::protocol_control_dev_parse(QTcpSocket *sock, MESPack &pack)
{
	MESControlRequest request;
	if (!request.decode(pack))
		return;
	emit q->sig_protocol_control_dev(request);
}

//ii.	下发公告
void MESNetPrivate::protocol_mail_parse(QTcpSocket *sock, MESPack &pack)
{
	MESPack pk(0xD8 - 0x80);
	MESNoticeRequest request;
	if (!request.decode(pack))
		return;
	emit q->sig_protocol_mail(request);
	pk.writeQUint8(0x00);
    sock->write(pk.toByteArray());
}

//////////////////////////////////////////////////////////////////

int MESNetPrivate::addRequestA(MESRequest *request)
{
	QByteArray arr = request->toMESPack().toByteArray();
	qDebug() << "请求A包:"<<MESTool::CSCPPackToQString(arr);

	aBuff.append(new QByteArray(arr));
	QMetaObject::invokeMethod(this, "_q_startNextRequestA",
			Qt::QueuedConnection);

	return 1;
}

void MESNetPrivate::_q_startNextRequestA()
{
	QByteArray *arr = aBuff.first();
	socketA->write(*arr);
	aBuff.removeFirst();
	delete arr;
}

int MESNetPrivate::addRequestB(MESRequest *request)
{
	QByteArray arr = request->toMESPack().toByteArray();
	qDebug() << "请求B包:"<<MESTool::CSCPPackToQString(arr);
	socketB->write(arr);
	return 1;
}

void MESNetPrivate::addResponseB(MESResponse *resposne)
{
	QByteArray arr = resposne->toMESPack().toByteArray();
	qDebug() << "响应B包:"<<MESTool::CSCPPackToQString(arr);
	socketB->write(arr);
}
