/*
 * CSCPAction.h
 *
 *网络通信的业务
 *  Created on: 2015年3月26日
 *      Author: cj
 */

#ifndef CSCPACTION_H_
#define CSCPACTION_H_

#include "../Public/public.h"
#include "mesrequest.h"

/**
 * 网络通信的业务 只有一个业务
 *
 */
class CSCPAction: public QObject
{
	static CSCPAction instance;Q_OBJECT
public:
	static CSCPAction& GetInstance();

	/**
	 * 派单
	 */
	bool OnOrderSend(Order &order, QList<entity::Material> &MaterialList);

	/**
	 * 删除工单
	 */
	bool OnOrderDelete(const QString &no);

	/**
	 * 设置时间
	 */
	bool OnSetSystemTime(const QDateTime &dateTime);

	/**
	 * 设置密码
	 */
	bool OnSetDevPassword(const QString &pass);
	/**
	 * 调整工单
	 */
	bool OnOrderModify(int index, const QString &DispatchNo,
			const QString &DispatchPrior);

	/**
	 * 设备换单
	 */
	bool OnOrderReplace();

	/**
	 * SQLite语句下发
	 */
	bool OnSQL(const QString &sql);

	/**
	 * 配置文件初始化
	 */
	bool OnConfigFileInit(const QString &fileName);

	/**
	 * 设置班次
	 */
	bool OnSetClassInfo(QList<MESControlRequest::ClassInfo> &);

	/**
	 * 网络连接消息
	 */
	void OnCSCPConnect(bool flag, const QString &mess);
	/**
	 * 配置文件下载完成
	 */
	void OnConfigDownSucess(const QString &name);

	/**
	 * 服务器下发公告
	 */
	void OnMailMess(QString startT, QString endT, QString mess);

	//信号
Q_SIGNALS:
	void OnSignalMESMailMess(QString startT, QString endT, QString mess);
	//网络连接是否正常信号
	void OnSignalCSCPConnect(bool flag, const QString &mess);
	//工单表改变信号
	void OnSignalOrderTableChange();
	//配置文件刷新
	void OnSignalConfigRefurbish(const QString &name);
};

#endif /* CSCPACTION_H_ */
