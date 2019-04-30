#include "jzlxsapplication.h"


JZLXSApplication *JZLXSApplication::instance;

JZLXSApplication *JZLXSApplication::getInstance()
{
    return instance;
}

JZLXSApplication::JZLXSApplication(int &argc, char **argv):QApplication(argc,argv)
{
    instance=this;
    rtspser=new QRTSPServer(this);
    systemInfo=new QSystemInfo();
    systemInfo->start();
}

JZLXSApplication::~JZLXSApplication()
{
    systemInfo->requestInterruption();
    systemInfo->quit();
    systemInfo->wait(100);

    rtspser->requestInterruption();
    rtspser->stopRtspServer();

    delete systemInfo;
    delete rtspser;
}
