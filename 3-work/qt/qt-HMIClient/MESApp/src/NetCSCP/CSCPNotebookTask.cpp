/*
 * CSCPNotebookTask.cpp
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#include "../Sqlite/SQLiteBaseHelper.h"
#include "../Unit/MESLog.h"
#include "CSCPNotebookTask.h"
#include "mesfiledown.h"

using namespace sqlite;

AbstractTask::AbstractTask(QObject* parent) :
		QObject(parent)
{
	subimtFlag = false;
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), this, SLOT(_slot_subimtTimeOut()));
}

AbstractTask::~AbstractTask()
{
	delete timer;
}

void AbstractTask::_slot_subimtTimeOut()
{
	qDebug() << "this Task TimeOut:";
	requestFinished(false);
}

void AbstractTask::start()
{
	timer->start(2000);
}

void AbstractTask::requestFinished(bool flag)
{
	subimtFlag = true;
	//停止运行定时器
	if (timer->isActive())
		timer->stop();
}

///////////////////////////////////////////////////////////////////////
template<typename T>
void showClassValueInfoList(const QString &str, Property &item)
{
	QList<T> *list = (QList<T> *) item.getVarPoint();
	for (int i = 0; i < list->size(); i++)
		showClassValueInfo(str + "\t", (*list)[i]);
}

void showClassValueInfo(const QString &str, AbsPropertyBase &data)
{
//	QList<Property> &propertylist = data.getPropertys();
//	logDebug("%sclass:%s", str.toAscii().data(), data.getClassName());
//
//	for (int i = 0; i < propertylist.size(); i++)
//	{
//		Property &item = propertylist[i];
//		logDebug("%sName:[%s] value:", str.toAscii().data(), item.getVarName());
//		switch (item.getType())
//		{
//		case Property::AsNone:
//		case Property::AsInt:
//			logDebug("%s[%d]", str.toAscii().data(), item.toInt());
//			break;
//		case Property::AsQStr:
//			logDebug("%s[%s]", str.toAscii().data(),
//					item.toQString().toAscii().data());
//			break;
//		case Property::AsEntity:
//			logDebug("=========================================");
//			showClassValueInfo(str + "\t", item.toObject<AbsPropertyBase>());
//			break;
//		case Property::AsQListInt:
//			logDebug("=========================================");
//			{
//				QList<int> *list = (QList<int>*) item.getVarPoint(); //item.toObject<QList<int>>();
//				for (int i = 0; i < list->size(); i++)
//					logDebug("%s,[%d]", str.toAscii().data(), list->value(i));
//			}
//			break;
//		case Property::AsQList_ItemProduct:
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<OtherSetInfo::ItemProduct>(str, item);
//		}
//			continue;
//			break;
//		case Property::AsQList_ItemMaterial:
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<OtherSetInfo::ItemMaterial>(str, item);
//		}
//			break;
//		case Property::AsQList_InspectionProj:	//点检项目
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<OtherSetInfo::InspectionProj>(str, item);
//		}
//			break;
//		case Property::AsQList_AdjustData: //调机数据
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<ADJMachine::AdjustData>(str, item);
//		}
//			break;
//		case Property::AsQList_ProductInfo: //每件产品的数据格式
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<DefectiveInfo::ProductInfo>(str, item);
//		}
//			break;
//		case Property::AsQList_FuncData: //每件产品的记录数据
//		{
//			logDebug("=========================================");
//			showClassValueInfoList<QualityRegulate::FuncData>(str, item);
//		}
//			break;
//		}
//	}
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CSCPNotebookTask *CSCPNotebookTask::instance;

//////////////////////////////////////////////////////////////////////
CSCPNotebookTask::CSCPNotebookTask(QObject *parent) :
		QObject(parent)
{
	instance = this;
	net = new MESNet();
	connect(MESNet::getInstance(),
			SIGNAL(signalReg(bool,const QDateTime,MESPack::FDATResult)), this,
			SLOT(onSlotReg(bool,const QDateTime,MESPack::FDATResult)));

	connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_control_dev(MESControlRequest&)), this,
			SLOT(slot_protocol_control_dev(MESControlRequest&)));

	connect(MESNet::getInstance(), SIGNAL(sig_protocol_order(MESOrderRequest&)),
			this, SLOT(slot_protocol_order(MESOrderRequest&))); //请求 派单
	connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_mail(MESNoticeRequest &)), this,
			SLOT(slot_protocol_mail(MESNoticeRequest&)));

	connect(MESNet::getInstance(), SIGNAL(sigCloseA()), this,
			SLOT(slot_closeA()));

	connect(MESNet::getInstance(), SIGNAL(sigConnectBSucess()), this,
			SLOT(slot_ConnectBSucess()));

	timer = 0;
	task1 = 0;
	task2 = 0;
}

CSCPNotebookTask::~CSCPNotebookTask()
{
	delete net;
}

CSCPNotebookTask& CSCPNotebookTask::GetInstance()
{
	return *instance;
}

void CSCPNotebookTask::startCSCPServer(const QString& ip, quint16 portA,
		quint16 portB)
{
	RegeditOK = false;
	if (timer == 0)
		timer = startTimer(500);
	MESNet::getInstance()->init(ip, portA, portB);
	MESNet::getInstance()->start();
}

void CSCPNotebookTask::closeCSCPServer()
{
	killTimer(timer);
	timer = 0;
	MESNet::getInstance()->close();
	RegeditOK = false;
}

bool CSCPNotebookTask::getConnectStatusA()
{
	return MESNet::getInstance()->stateA() == MESNet::Connected;
}

bool CSCPNotebookTask::getConnectStatusB()
{
	return MESNet::getInstance()->stateB() == MESNet::Connected;
}

//请求 派单
void CSCPNotebookTask::slot_protocol_order(MESOrderRequest& request)
{
	CSCPAction::GetInstance().OnOrderSend(request.order, request.MaterialList);
	MESOrderResponse response(request.FDAT_DataType,
			request.order.getMoDispatchNo(), request.order.getMoDispatchPrior(),
			0x00);
	MESNet::getInstance()->sendMESOrderResponse(response);
}

void CSCPNotebookTask::slot_protocol_mail(MESNoticeRequest &request)
{
    logDebug("下发公告:"+request.Text);
	CSCPAction::GetInstance().OnMailMess(
			Tool::DateTimeToQString(request.StartTime),
			Tool::DateTimeToQString(request.EndTime), request.Text);
}

//请求 控制设备
void CSCPNotebookTask::slot_protocol_control_dev(MESControlRequest &request)
{
	MESControlResponse response(request.FDAT_DataType, 0x00,
			request.FDAT_CmdCode);
	switch (request.FDAT_DataType)
	{
	case 0x00: //0x00：设备复位
		break;
	case 0x01: //0x01：读取设备RTC时间
		response.rtcTime = QDateTime::currentDateTime();
		break;
	case 0x02: //0x02：设置设备RTC时间
		CSCPAction::GetInstance().OnSetSystemTime(request.rtcDateTime);
		break;
	case 0x03: //0x03：读取设备剩余FLASH空间
		break;
	case 0x04: //0x04: 下载文件
	{
		MESFileDown *fileDown = new MESFileDown(request.FDAT_FileName,
				request.FDAT_FileVer);
		fileDown->start();
	}
		break;
	case 0x05: //0x05: 上传文件
		break;
	case 0x06: //0x06: 设置设备密码
		CSCPAction::GetInstance().OnSetDevPassword(request.devPassword);
		break;
	case 0x07: //0x07: 调整工单
	{
		for (int i = 0; i < request.orderIndexList.size(); i++)
			CSCPAction::GetInstance().OnOrderModify(
					request.orderIndexList[i].FDAT_SerialNumber,
					request.orderIndexList[i].FDAT_DispatchNo,
					request.orderIndexList[i].FDAT_DispatchPrior);
	}
		break;
		//case 0x08:
		//break; //0x08: 读取程序最新版本号
	case 0x09:		//0x09: 设备换单
		CSCPAction::GetInstance().OnOrderReplace();
		break;
	case 0x0A:		//0x0A: 设置标准温度
		break;
	case 0x0B:		//0x0B：设置SQLite语句
		CSCPAction::GetInstance().OnSQL(request.sql);
		break;
	case 0x0C:		//0x0C: 配置文件初始化
		CSCPAction::GetInstance().OnConfigFileInit(request.initFileName);
		break;
	case 0x0D:		//0x0D: 设置班次
		CSCPAction::GetInstance().OnSetClassInfo(request.classList);
		break;
	case 0x0E:		//工单驳回
		CSCPAction::GetInstance().OnOrderDelete(request.FDAT_DispatchNo);
		break;
	}
	response.bindRequest(request);
	MESNet::getInstance()->addMESControlResponse(response);
}
//同步工单
bool CSCPNotebookTask::OnAsyncOrder()
{
	QList<Order> list;
	Order::queryAll(list);
	QStringList myOrderDispatchNo;
	for (int i = 0; i < list.size(); i++)
		myOrderDispatchNo.append(list[i].getMoDispatchNo());
	///////////////////////////////////////////////////////////////
	MESOtherRequest request;
	request.setAsyncOrder(myOrderDispatchNo);
	MESNet::getInstance()->sendRequestMESOtherRequest(request);
	return false;
}

void CSCPNotebookTask::timerEvent(QTimerEvent *e)
{
	runTask();
}

void CSCPNotebookTask::runTask()
{
	if (RegeditOK)
	{
		if (Producted::count() > 0)
		{
			if (task1 == 0)
			{
				task1 = new TaskProducted();		//生产数据 上传模次
				task1->start();
			}
			else
			{
				if (task1->subimtFlag)
				{
					delete task1;
					task1 = 0;
				}
			}
		}

		if (Notebook::count() > 0)
		{
			if (task2 == 0)
			{
				task2 = new TaskNotebook();		//生产数据 上传操作
				task2->start();
			}
			else
			{
				if (task2->subimtFlag)
				{
					delete task2;
					task2 = 0;
				}
			}
		}
	}

	//同步信号
}

void CSCPNotebookTask::onSlotReg(bool flag, const QDateTime str,
		MESPack::FDATResult result)
{
	if (flag)
	{
		CSCPAction::GetInstance().OnCSCPConnect(true, "网络注册成功");
		CSCPAction::GetInstance().OnSetSystemTime(str); //修改时间
		RegeditOK = true; //有数据就上传,没有就等待一个时间后上传
	}
	else
	{
		RegeditOK = false;
		CSCPAction::GetInstance().OnCSCPConnect(false, "网络注册失败");
	}
}

void CSCPNotebookTask::slot_closeA()
{
	RegeditOK = false;
}

void CSCPNotebookTask::slot_ConnectBSucess() //B端口连接成功后,检查是否下载文件
{
	QString cachePath = AppInfo::getPath_DownCache();
	QDir dir(cachePath);
	if (dir.exists())
	{
		QFileInfoList infoList = dir.entryInfoList();
		for (int i = 0; i < infoList.size(); i++)
		{
			if (infoList[i].isFile())
			{
				///////////////////
				MESFileDown *fileDown = new MESFileDown(infoList[i].fileName(),
						QByteArray());
				fileDown->start();
			}
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void TaskNotebook::start()
{
	subimtFlag = false;
//上传其它 数据的
	if (SQLiteBaseHelper::getInstance().selectItem(notebook, "*", "", "id"))
	{
		if (subimt())
			AbstractTask::start();
		else
			requestFinished(false);
		return;
	}
	requestFinished(false);
}
void TaskNotebook::requestFinished(bool flag)
{
	if (flag)
	{		
		if (notebook.remove())
			logDebug("上传数据 删除OK");
		CSCPAction::GetInstance().OnCSCPConnect(true, notebook.getTitle());
	}
	AbstractTask::requestFinished(flag);
}

bool TaskNotebook::subimt()	//分解当前数据是哪种类型,从而提交上
{
    logDebug(
            QString("网络上传数据：[%1]\n%2").arg(notebook.getTitle()).arg(
                    notebook.getText()));

	if (notebook.getType().compare(BrushCard::GetThisClassName()) == 0) //刷卡上传
	{
		BrushCard data;
		XMLToPropertyBase(notebook.getText(), data);
		showClassValueInfo("", data);
		return subimt_0x11(data);
	}

//次品上传
	if (notebook.getType().compare(DefectiveInfo::GetThisClassName()) == 0)
	{
		DefectiveInfo data;
		XMLToPropertyBase(notebook.getText(), data);
		showClassValueInfo("", data);
		return subimt_0x13(data);
	}
//ii.	发送调机良品数信息
	if (notebook.getType().compare(ADJMachine::GetThisClassName()) == 0)
	{
		ADJMachine data;
		XMLToPropertyBase(notebook.getText(), data);
		showClassValueInfo("", data);
		return subimt_0x14(data);
	}

//发送其它设置信息
	if (notebook.getType().compare(OtherSetInfo::GetThisClassName()) == 0)
	{
		XMLToPropertyBase(notebook.getText(), otherSetInfo);
		showClassValueInfo("", otherSetInfo);
		return subimt_0x15(otherSetInfo);
	}

//功能记录（巡机数、打磨数等）信息
	if (notebook.getType().compare(QualityRegulate::GetThisClassName()) == 0)
	{
		QualityRegulate data;
		XMLToPropertyBase(notebook.getText(), data);
		showClassValueInfo("", data);
		return subimt_0x16(data);
	}
	return false;
}

bool TaskNotebook::subimt_0x11(BrushCard& brushCard)
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_ic_card(quint8,MESPack::FDATResult)), this,
			SLOT(slot_protocol_ic_card(quint8,MESPack::FDATResult))))
		return false;
	MESBrushCardRequest request(brushCard);
	if (MESNet::getInstance()->sendRequestMESBrushCardRequest(request) == 0)
		return false;
	return true;
}

bool TaskNotebook::subimt_0x13(DefectiveInfo& info)
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_defectiveInfo(MESPack::FDATResult)), this,
			SLOT(slot_protocol_defectiveInfo(MESPack::FDATResult))))
		return false;
	MESDefectiveInfoRequest request(info);
	if (MESNet::getInstance()->sendRequestMESDefectiveInfoRequest(request) == 0)
		return false;
	return true;
}

bool TaskNotebook::subimt_0x14(ADJMachine& info)
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_adj_machine(MESPack::FDATResult)), this,
			SLOT(slot_protocol_adj_machine(MESPack::FDATResult))))
		return false;

	if (MESNet::getInstance()->sendRequestADJMachine(info) == 0)
		return false;
	return true;
}

bool TaskNotebook::subimt_0x15(OtherSetInfo& info)
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_other(const MESOtherResponse&)), this,
			SLOT(slot_protocol_other(const MESOtherResponse&))))
		return false;
	MESOtherRequest request(info);
	if (MESNet::getInstance()->sendRequestMESOtherRequest(request) == 0)
		return false;
	return true;
}

bool TaskNotebook::subimt_0x16(QualityRegulate& info)
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_QualityRegulate(MESPack::FDATResult)), this,
			SLOT(slot_protocol_QualityRegulate(MESPack::FDATResult))))
		return false;
	MESQualityRegulateRequest request(info);
	if (MESNet::getInstance()->sendRequestMESQualityRegulate(request) == 0)
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void TaskNotebook::slot_protocol_ic_card(quint8 type,
		MESPack::FDATResult result)
{
    if(result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<<notebook.getTitle()<<" 响应出错:"<<result<<endl;

	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}

void TaskNotebook::slot_protocol_defectiveInfo(MESPack::FDATResult result)
{
    if(result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<<notebook.getTitle()<<" 响应出错:"<<result<<endl;

	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}

void TaskNotebook::slot_protocol_adj_machine(MESPack::FDATResult result)
{
    if(result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<<notebook.getTitle()<<" 响应出错:"<<result<<endl;

	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}

void TaskNotebook::slot_protocol_other(const MESOtherResponse& response)
{
    if(response.Result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<<notebook.getTitle()<<" 响应出错:"<<response.Result<<endl;

	if (response.Result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
	{
		requestFinished(false);
		return;
	}
	if (otherSetInfo.getDataType() == response.FDAT_DataType)
	{
		requestFinished(true);
	}
//		switch (response.FDAT_DataType)
//		{
//		case entity::OtherSetInfo::AsUpgradeFile: // = 0x00, //：升级文件
//		case entity::OtherSetInfo::AsAsyncOrder: // = 0x01, //：同步工单
//		case entity::OtherSetInfo::AsAdjOrder: // = 0x02, //：调整工单
//		case entity::OtherSetInfo::AsAdjSock: // = 0x03, //：调整模穴数
//		case entity::OtherSetInfo::AsAdjMaterial: // = 0x04, //：调整原料
//		case entity::OtherSetInfo::AsCardStopNo: // = 0x05, //：刷完停机卡计数
//		case entity::OtherSetInfo::AsDevInspection: // = 0x06, //：设备点检
//		case entity::OtherSetInfo::AsELEFeeCount: // = 0x07电费统计
//		}
}

void TaskNotebook::slot_protocol_QualityRegulate(MESPack::FDATResult result)
{
    if(result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<<notebook.getTitle()<<" 响应出错:"<<result<<endl;

	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}

void TaskNotebook::slot_protocol_pda(MESPack::FDATResult result)
{
	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void TaskProducted::start()
{
	if (SQLiteProductedHelper::getInstance().selectItem(producted, "*", "",
			"id"))
	{
		if (subimt())
			AbstractTask::start();
		else
			requestFinished(false);
		return;
	}
	requestFinished(false);
}

bool TaskProducted::subimt()
{
	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_produce(quint8,MESPack::FDATResult)), this,
			SLOT(slot_protocol_produce(quint8,MESPack::FDATResult))))
		return false;
	MESProduceRequest request(this->producted);
	if (MESNet::getInstance()->sendRequestMESProduce(request) == 0)
		return false;
	return true;
}

void TaskProducted::requestFinished(bool flag)
{
	if (flag)
	{
		qDebug() << "模次上传完成";
		producted.subimt_bak();
		producted.remove(); //删除
	}
//判断生产数据备份表的数据长度 大于n时就删掉记录
	if (Producted::count_bak()
			> AppInfo::GetInstance().getTbProductedBakCount())
	{
		SQLiteProductedHelper::getInstance().exec(
				"delete from tb_Producted_bak");
	}
	AbstractTask::requestFinished(flag);
}

void TaskProducted::slot_protocol_produce(quint8 type,
		MESPack::FDATResult result)
{
    if(result!=MESPack::FDAT_RESULT_SUCESS)
        logWarn()<< "生产 响应出错:"<<result<<endl;

	if (result == MESPack::FDAT_RESULT_FAIL_CLOSE) //网络己断开
		requestFinished(false);
	else
		requestFinished(true);
}

