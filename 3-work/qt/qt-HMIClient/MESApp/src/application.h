#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <QApplication>
#include "Server/rfserver.h"
#include "Server/collection.h"
#include "Server/CollectCycle.h"
#include "Public/ButtonStatus.h"

class NetCommandService;
class ApplicationPrivate;

// 应用程序
class Application : public QApplication
{
	static Application *self;
    Q_OBJECT

public:
	Application(int& argc, char** argv);
	~Application();

	static Application *instance() { return self; }

    // 初始化
    void init();

    bool notify(QObject* obj, QEvent *e);

    void MESExit();

private:
    ApplicationPrivate* d;
};

class ApplicationPrivate
{
public:
    ApplicationPrivate();
    ~ApplicationPrivate();

    //日志初始化
    void logInit();
    // APP初始化
    void appInit();
    //RFID
    void RFIDInit();
    //485初始化
    void RS485Init();
    //计数数据
    void colectcyclInit();


public:
    UIPageButtonStatus *allFunButtonStatus;
    RFIDServer *rfidserver;
    Collection *rs485server;
    CollectCycle * collectcycle;
    NetCommandService *netcmdserver;
    CSCPNotebookTask *NotebookTask;
   // Backlight* pbacklight;
};

#endif // _APPLICATION_H_
