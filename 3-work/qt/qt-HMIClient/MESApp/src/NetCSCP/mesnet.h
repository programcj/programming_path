#ifndef MESNET_H
#define MESNET_H

#include <QObject>
#include <QTcpSocket>

#include "mespack.h"
#include "mestool.h"
#include "mesrequest.h"

class MESNetPrivate;

//网络通信接口操作类
class MESNet: public QObject
{
	static MESNet *instance;

    Q_ENUMS(State)
Q_OBJECT
public:
	enum State
	{
		Unconnected, Connecting, Connected, Closing
	};

	explicit MESNet(QObject *parent = 0);
	~MESNet();
	void init(const QString &ip, qint16 portA, qint16 portB);

	void start();
	void close();

	int sendRequestADJMachine(entity::ADJMachine &v);
	int sendRequestFileDownStart(const QString &name, const QByteArray &ver);
	int sendRequestFileDownStream(MESFileDownStreamRequest &request);
	int sendRequestMESOtherRequest(MESOtherRequest &);
	int sendRequestMESQualityRegulate(MESQualityRegulateRequest &request);
	int sendRequestMESDefectiveInfoRequest(MESDefectiveInfoRequest &request);
	int sendRequestMESBrushCardRequest(MESBrushCardRequest &request);
	int sendRequestMESProduce(MESProduceRequest &request);

    int sendMESOrderResponse(MESOrderResponse &response);

	static MESNet *getInstance();

	State stateA();
	State stateB();

	MESPack::FDATResult getRegStatus();

	void addMESControlResponse(MESControlResponse &response);
///////////////////////////////////////////////////////////////////////////
signals:

	void sigCloseA();
	void sigCloseB();
	void sigConnectBSucess();

	//注册 完成后的信号
	void signalReg(bool, const QDateTime serverTime,
			MESPack::FDATResult result); //注删响应

	void sig_protocol_produce(quint8 type, MESPack::FDATResult result); //生产数据响应
	void sig_protocol_ic_card(quint8 type, MESPack::FDATResult result); //ii.	发送刷卡数据 响应
	void sig_protocol_defectiveInfo(MESPack::FDATResult result); //iii.	发送次品信息 响应
	void sig_protocol_adj_machine(MESPack::FDATResult result); //调机 响应
	void sig_protocol_other(const MESOtherResponse &); //ii.发送其它设置信息 响应

	void sig_protocol_QualityRegulate(MESPack::FDATResult result); //巡机数、打磨数 响应
	void sig_protocol_pda(MESPack::FDATResult result);	//PDA操作方面的 87
	void sig_protocol_wifi_signal(MESPack::FDATResult result); //同步信号 响应 D0
	//文件下载专用
	void sig_MESFileDownResponse(MESFileDownResponse);
	void sig_MESFileDownStreamResponse(MESFileDownStreamResponse&);
	//服务器请求专用
	void sig_protocol_order(MESOrderRequest &request);  //请求 派单
	void sig_protocol_control_dev(MESControlRequest &request);  //请求 控制设备
    void sig_protocol_mail(MESNoticeRequest &request); //ii.	下发公告

private:
	MESNetPrivate *d;
	friend class MESNetPrivate;
	friend class MESRegRequest;
	friend class MESADJMachineRequest;
	friend class MESControlRequest;
};

//私有类
class MESNetPrivate: public QObject
{
    MESNet *q;
    QTimer *regTimeOut;

    Q_OBJECT
public:
	MESNetPrivate(MESNet *net)
	{
		this->q = net;
		stateA = MESNet::Unconnected;
		stateB = MESNet::Unconnected;

		socketA = 0;
		socketB = 0;
		portA = 0;
		portB = 0;
        regTimeOut = new QTimer(this);
        regTimeOut->setSingleShot(true);
        connect(regTimeOut, SIGNAL(timeout()), this, SLOT(_slot_regTimeOut()));
	}

	inline ~MESNetPrivate()
	{

	}
	bool startFlag;
	void init();
	void start();
	void close();

	QString serverIP;
	qint16 portA;
	qint16 portB;

	QTcpSocket *socketA;
	QTcpSocket *socketB;

	MESNet::State stateA;
	MESNet::State stateB;

	QByteArray buffReadA;
	QByteArray buffReadB;

	void parseReadBuff(QTcpSocket *, QByteArray &buff);

	///////////////////////////////////////////////////////////////////////////////////////////////
	void sendMESADJMachine(MESADJMachineRequest *request);

	int addRequestA(MESRequest *request);
	int addRequestB(MESRequest *request);

	QList<QByteArray*> aBuff;

	void addResponseB(MESResponse *resposne);
	///////////////////////////////////////////////////////////////////////////////////////////////
	void protocol_regdev_parse(QTcpSocket *, MESPack &); //注删响应
	void protocol_produce_parse(QTcpSocket *, MESPack &); //生产数据响应
	void protocol_ic_card_parse(QTcpSocket *, MESPack &); //ii.	发送刷卡数据 响应
	void protocol_defectiveInfo_parse(QTcpSocket *, MESPack &); //iii.	发送次品信息 响应
	void protocol_adj_machine_parse(QTcpSocket *, MESPack &); //调机 响应
	void protocol_other_parse(QTcpSocket *, MESPack &); //ii.发送其它设置信息 响应
	void protocol_QualityRegulate_parse(QTcpSocket *, MESPack &); //巡机数、打磨数 响应
	void protocol_file_down_parse(QTcpSocket *, MESPack &); //ii.	下载文件
	//void protocol_file_down_parse(QTcpSocket *,MESPack &); //i.	启动下载文件 0x40
	void protocol_pda_parse(QTcpSocket *, MESPack &);	//PDA操作方面的 87
	void protocol_wifi_signal_parse(QTcpSocket *, MESPack &); //同步信号 响应 D0
	void protocol_order_parse(QTcpSocket *, MESPack &);  //请求 派单
	void protocol_control_dev_parse(QTcpSocket *, MESPack &);  //请求 控制设备
	void protocol_mail_parse(QTcpSocket *, MESPack &); //ii.	下发公告

	//注册请求 上传到服务器
	MESRegRequest regRequest;

private slots:
    void _slot_regTimeOut();

    void _q_restart();

	void _q_slotConnectedA();
	void _q_slotConnectedB();

	void _q_slotClosedA();
	void _q_slotClosedB();

	void _q_slotReadyReadA();
	void _q_slotReadyReadB();

	void _q_slotErrorA(QAbstractSocket::SocketError err);
	void _q_slotErrorB(QAbstractSocket::SocketError err);

	void _q_slotBytesWrittenA(qint64 written);
	void _q_slotBytesWrittenB(qint64 written);

	void _q_startNextRequestA();
};

#endif // MESNET_H
