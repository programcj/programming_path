#include "application.h"


Application *Application::self;

Application::Application(int& argc, char** argv) :
    QApplication(argc, argv)
{
    self = this;
    qDebug()<< "start..";

    //数据库初始化,数据库打开
    sqlite::SQLiteBaseHelper::getInstance().init(FILE_NAME_DATABASE, 0);
    //是否是新版本 //如果不等于数据库中的版本 就保存新的版本

    QString versino=sqlite::SQLiteBaseHelper::getInstance().getVersion("App_ChiefMES.linux");
    if(versino.length()>0 && versino != "1" )
    {   //须要设定成app文件升级成功
        qDebug()<< QString("运行的是最新的程序文件:%1").arg(1);
    }
    sqlite::SQLiteBaseHelper::getInstance().versionSave("App_ChiefMES.linux","1");

    haveStopCardFlag=false;

    mainDispatchOrder =new MESDispatchOrder(this);
    mesMqttClient = new MESMqttClient(this);
    mesMsgHandler = new MESMsgHandler(this);
    netBroadHandler =new NetBroadHandler(this);
    serviceDICycle = new ServiceDICycle(this);
    serviceModuleKang = new ServiceModuleKang(this);
}

Application::~Application()
{
    delete mesMqttClient;
    delete mesMsgHandler;
    delete mainDispatchOrder;
    delete netBroadHandler;
    delete serviceDICycle;
    delete serviceModuleKang;
}

void Application::init()
{
    qDebug() << "init";

    MESMsgHandler::getInstance()->start(); //消息处理任务(线程)

    serviceModuleKang->setUart(AppConfig::getInstance().serviceModuleKangUart);
    serviceModuleKang->start();    //康耐德串口通讯服务

    ServiceDICycle::getInstance()->start(); //DI采集

    //设定Mqtt服务器地址
    MESMqttClient::getInstance()->url=AppConfig::getInstance().mqttURL;
    MESMqttClient::getInstance()->userName=AppConfig::getInstance().mqttUser;
    MESMqttClient::getInstance()->password= AppConfig::getInstance().mqttPassword;
    MESMqttClient::getInstance()->start(); //启动

    qDebug()<< "init end";
}

void Application::restartServiceModleKang()
{
    QMutexLocker locker(&mutex);

    serviceModuleKang->requestInterruption();
    serviceModuleKang->wait();

    qDebug()<<"康耐德串口通讯服务 restart";

    serviceModuleKang->setUart(AppConfig::getInstance().serviceModuleKangUart);
    serviceModuleKang->start();    //康耐德串口通讯服务
}

void Application::restartServiceMqttService(){
    QMutexLocker locker(&mutex);

    qDebug()<<"MQTT通讯服务 restart";

    Application::getInstance()->mesMqttClient->url=AppConfig::getInstance().mqttURL;
    Application::getInstance()->mesMqttClient->userName=AppConfig::getInstance().mqttUser;
    Application::getInstance()->mesMqttClient->password=AppConfig::getInstance().mqttPassword;
    Application::getInstance()->mesMqttClient->restartConnect();
}

void Application::stopAndExit()
{
    serviceDICycle->requestInterruption();  //DI采集停止
    mesMqttClient->requestInterruption(); //MQTT通讯停止
    serviceModuleKang->requestInterruption(); //温度与压力停止
    mesMsgHandler->requestInterruption2();  //消息处理停止

    sqlite::SQLiteBaseHelper::getInstance().close();

    ::exit(0);
    serviceDICycle->wait();
    serviceModuleKang->wait();
    mesMsgHandler->wait();
    mesMqttClient->wait();
}
