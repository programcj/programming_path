#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QDebug>
#include "public.h"

class AppConfig
{
    static AppConfig *self;

public:
    AppConfig();

    QString mqttURL;
    QString mqttUser;
    QString mqttPassword;
    QString AppVersion;
    QString DeviceSN;

    QString serviceModuleKangUart; //康乃德服务对应的串口

    static AppConfig &getInstance();

    void save();
};

#endif // APPCONFIG_H
