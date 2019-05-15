#ifndef APPLICATION_H
#define APPLICATION_H

#include <QtCore>
#include <QtNetwork>
#include <QDebug>
#include <QApplication>
#include "dao/sqlitebasehelper.h"
#include "dao/mesdispatchorder.h"

#include "netinterface/mesmqttclient.h"
#include "netinterface/mesmsghandler.h"
#include "netinterface/netbroadhandler.h"

#include "hal/servicedicycle.h"
#include "hal/servicemodulekang.h"
#include "public.h"
#include "appconfig.h"

class Application : public QApplication
{
    static Application *self;
    QMutex mutex;

    Q_OBJECT
public:
    Application(int& argc, char** argv);
    ~Application();

    AppConfig appConfig;
    sqlite::SQLiteBaseHelper sqliteBaseHelp;

    MESMqttClient *mesMqttClient;
    MESMsgHandler *mesMsgHandler;
    NetBroadHandler *netBroadHandler;

    //DI采集服务
    ServiceDICycle *serviceDICycle;
    //康耐德串口通讯服务 -- 温度与压力
    ServiceModuleKang *serviceModuleKang;

    bool haveStopCardFlag;

    /**
     * 主单
     * @brief mainDispatchOrder
     */
    MESDispatchOrder *mainDispatchOrder;

    static Application *getInstance() { return self; }

    // 初始化
    void init();

    void restartServiceModleKang();

    void restartServiceMqttService();

    void stopAndExit();

signals:

public slots:

};

#endif // APPLICATION_H
