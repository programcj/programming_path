#include <QtDebug>
#include <QApplication>
#include "application.h"
#include "Server/rfserver.h"
#include "Server/collection.h"
#include "Server/CollectCycle.h"
#include "AppConfig/AppInfo.h"
#include "Action/mainwindow.h"
#include "Action/inputpannelcontext.h"
#include <QNetworkInterface>
#include "Server/NetCommandService.h"
#include "HAL/backlight/Backlight.h"

Application *Application::self;

Application::Application(int& argc, char** argv) :
		QApplication(argc, argv), d(new ApplicationPrivate())
{
	self = this;
}

Application::~Application()
{
	delete d;
}

// 初始化（初始化顺序比较重要，因为对象间有依赖关系，所以更改时要小心）
void Application::init()
{
	d->logInit();
	d->appInit();
    d->RFIDInit();
    d->RS485Init();
    d->colectcyclInit();
}



ApplicationPrivate::ApplicationPrivate() :
		rfidserver(0), rs485server(0), collectcycle(0), netcmdserver(0)
{
	NotebookTask = new CSCPNotebookTask();
}

ApplicationPrivate::~ApplicationPrivate()
{
	//释放线程功能
	delete netcmdserver;
	delete rfidserver;
	delete collectcycle;
	delete rs485server;
	delete NotebookTask;
}

// APP初始化
void ApplicationPrivate::appInit()
{
	//初始化日志目录
	MESLog::initPaths(AppInfo::getPath_Logs());

	//数据库初始化
	SQLiteBaseHelper::getInstance().init(
			AppInfo::getPath_Database() + "chiefmes_Base.db", 0);
	SQLiteProductedHelper::getInstance().init(
			AppInfo::getPath_Database() + "chiefmes_Producted.db", 0);

	OrderMainOperation::GetInstance().init();
	OrderMainOperation::GetInstance().OnUpdate(); //主单更新

	//配置初始化
	AppInfo::GetInstance().init();
	AppInfo::GetInstance().initNetCSCP();
    //其它初始化
    allFunButtonStatus = new UIPageButtonStatus(); //当前页所有的按钮信息
	netcmdserver = new NetCommandService();

    //是否是新版本 //如果不等于数据库中的版本 就保存新的版本

    QString versino=sqlite::SQLiteBaseHelper::getInstance().getVersion("App_ChiefMES.linux");

    if(versino.length()>0 && versino != AppInfo::GetInstance().getVersion() )
    {
        //须要设定成app文件升级成功
        logInfo("运行的是最新的程序文件:"+AppInfo::GetInstance().getVersion());
    }
    sqlite::SQLiteBaseHelper::getInstance().versionSave("App_ChiefMES.linux",AppInfo::GetInstance().getVersion());

    logDebug("初始化完成");

	{ //本机IP列表
		QList<QHostAddress> ipList = QNetworkInterface::allAddresses();
		for (int i = 0; i < ipList.size(); i++)
		{
            logDebug(ipList[i].toString());
		}
	}
}

void ApplicationPrivate::logInit()
{
    // 注册调试信息处理函数
  //  TraceLog::init();
   // qInstallMsgHandler(MESLog::outputMessage);
}

//RFID
void ApplicationPrivate::RFIDInit()
{
	rfidserver = RFIDServer::GetInstance();
    rfidserver->RFIDStart();
}
//485初始化
void ApplicationPrivate::RS485Init()
{
	rs485server = Collection::GetInstance();
	rs485server->ColetctStart();
}
//计数数据
void ApplicationPrivate::colectcyclInit()
{
	collectcycle = CollectCycle::GetInstance();
	collectcycle->start();
}

bool Application::notify(QObject* obj, QEvent* e)
{
	if (e->type() == QEvent::MouseButtonRelease && obj != 0)
	{
		if (obj->inherits("QPushButton"))
		{
			//如果有按钮声音
			if (AppInfo::GetInstance().isButtonBuzzer())
			{
				Tool::ExeBuzzer();
			}
		}
	}
	return QApplication::notify(obj, e);
}

void Application::MESExit()
{
    logInfo("系统退出");
	Collection::GetInstance()->ColetctStop();
	RFIDServer::GetInstance()->RFIDEnd();

	//数据采集处理
	CollectCycle::GetInstance()->interrupt();
	CollectCycle::GetInstance()->quit();

    //关闭数据库和网络接口
	CSCPNotebookTask::GetInstance().closeCSCPServer(); //关闭网络,再关数据库
	SQLiteBaseHelper::getInstance().close();
	SQLiteProductedHelper::getInstance().close();

	exit();
	//QApplication::exit();
}
