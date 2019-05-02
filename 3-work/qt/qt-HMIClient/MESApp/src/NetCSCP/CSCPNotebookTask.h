/*
 * CSCPNotebookTask.h
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#ifndef CSCPNOTEBOOKTASK_H_
#define CSCPNOTEBOOKTASK_H_

#include "../Sqlite/Entity/Notebook.h"
#include <QtCore>
#include <QString>
#include "mesnet.h"
#include "../Public/public.h"
#include "CSCPAction.h"

using namespace entity;

//任务提交抽象类
class AbstractTask:public QObject
{
    Q_OBJECT
public:
    explicit AbstractTask(QObject *parent = 0);
    ~AbstractTask();

    bool subimtFlag;
    virtual void start();

    virtual void requestFinished(bool flag);
private:
    QTimer *timer;
private slots:
    void _slot_subimtTimeOut();
};

//生产任务提交类
class TaskProducted: public AbstractTask
{
	Producted producted;Q_OBJECT
public:
	void start();
private:
	bool subimt();
	void requestFinished(bool flag);

private slots:
	void slot_protocol_produce(quint8 type, MESPack::FDATResult result); //生产数据响应
};

//数据任务提交类
class TaskNotebook: public AbstractTask
{
	Notebook notebook;
	OtherSetInfo otherSetInfo;

Q_OBJECT
public:
	void start();

private:
	bool subimt();
	void requestFinished(bool flag);

	//i.	发送生产数据 0x10
	//bool subimt_0x10(Producted &producted);
	//ii.	发送刷卡数据
	bool subimt_0x11(BrushCard &brushCard);
	//iii.	发送次品信息 命令序列号：值固定为0x13
	bool subimt_0x13(DefectiveInfo &info);
	//ii.	发送调机良品数信息 	命令序列号：值固定为0x14
	bool subimt_0x14(ADJMachine &info);
	//ii.	发送其它设置信息 0x15
	bool subimt_0x15(OtherSetInfo &info);
	//i.	发送功能记录（巡机数、打磨数等）信息  0x16
	bool subimt_0x16(QualityRegulate &info);

private slots:
	//void slot_protocol_produce(quint8 type, MESPack::FDATResult result); //生产数据响应
	void slot_protocol_ic_card(quint8 type, MESPack::FDATResult result); //ii.	发送刷卡数据 响应
	void slot_protocol_defectiveInfo(MESPack::FDATResult result); //iii.	发送次品信息 响应
	void slot_protocol_adj_machine(MESPack::FDATResult result); //调机 响应
	void slot_protocol_other(const MESOtherResponse &); //ii.发送其它设置信息 响应
	void slot_protocol_QualityRegulate(MESPack::FDATResult result); //巡机数、打磨数 响应
	void slot_protocol_pda(MESPack::FDATResult result);	//PDA操作方面的 87
	//void slot_protocol_wifi_signal(MESPack::FDATResult result); //同步信号 响应 D0
};
/*
 * 数据上传，刷卡等
 */
class CSCPNotebookTask: public QObject
{
	static CSCPNotebookTask *instance;
	TaskProducted *task1;
	TaskNotebook *task2;
	MESNet *net;Q_OBJECT
public:
	explicit CSCPNotebookTask(QObject *parent = 0);
	~CSCPNotebookTask();
	static CSCPNotebookTask &GetInstance();

	//开启-重启CSCP服务
	void startCSCPServer(const QString &ip, quint16 portA, quint16 portB);

	//关闭CSCP服务
	void closeCSCPServer();

	//取网络连接状态
	bool getConnectStatusA();
	bool getConnectStatusB();

	//同步工单操作 其实也就是获取服务器上的工单数据
    bool OnAsyncOrder();

	virtual void timerEvent(QTimerEvent *e);

private slots:
	void onSlotReg(bool flag, const QDateTime str, MESPack::FDATResult result);
	void slot_protocol_control_dev(MESControlRequest &request);  //请求 控制设备
	void slot_protocol_order(MESOrderRequest &request);
	void slot_protocol_mail(MESNoticeRequest &request);
	void slot_closeA();
	void slot_ConnectBSucess();

private:
	int timer;
	bool RegeditOK;
	void runTask();

	friend class TaskProducted;
	friend class TaskNotebook;
};

#endif /* CSCPNOTEBOOKTASK_H_ */
