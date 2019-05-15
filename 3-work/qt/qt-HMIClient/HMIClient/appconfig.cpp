#include "appconfig.h"

#include <QSettings>
#include <QFile>
#include "tools/mestools.h"

#define  CONFIG_MQTT_URL    "mqtt.url"
#define  CONFIG_MQTT_USER   "mqtt.user"
#define  CONFIG_MQTT_PASS   "mqtt.password"
#define  CONFIG_DEVICE_SN     "config.sn"
#define  CONFIG_SERVICE_KANG_UART "module.kang.uart"

AppConfig *AppConfig::self=NULL;

AppConfig::AppConfig()
{
    self=this;
    if(MESTools::fileIsExists(FILE_NAME_SYSCONFIG)) {
        QSettings *configIniRead = new QSettings(FILE_NAME_SYSCONFIG, QSettings::IniFormat);
        mqttURL = configIniRead->value(CONFIG_MQTT_URL).toString();
        mqttUser = configIniRead->value(CONFIG_MQTT_USER).toString();
        mqttPassword = configIniRead->value(CONFIG_MQTT_PASS).toString();
        DeviceSN = configIniRead->value(CONFIG_DEVICE_SN).toString();
        serviceModuleKangUart = configIniRead->value(CONFIG_SERVICE_KANG_UART).toString();
        delete configIniRead;

#ifdef Q_OS_LINUX_WEIQIAN
        if(serviceModuleKangUart=="/dev/ttyAMA0"){
            return;
        }
        if(serviceModuleKangUart=="/dev/ttyAMA1") {
            return;
        }
        serviceModuleKangUart="/dev/ttyAMA0";
#endif

#ifdef AM335X
        if(serviceModuleKangUart=="/dev/ttyO2"){
            return;
        }
        if(serviceModuleKangUart=="/dev/ttyO1") {
            return;
        }
        serviceModuleKangUart="/dev/ttyO2";
#endif
    } else {
        mqttURL="tcp://192.168.50.139:61613";
        mqttUser="admin";
        mqttPassword="password";
        DeviceSN="444";
#ifdef  Q_OS_LINUX_WEIQIAN
    serviceModuleKangUart="/dev/ttyAMA0";
#endif
#ifdef AM335X
    serviceModuleKangUart="/dev/ttyO2"; //232
#endif
        save();
    }
}

AppConfig &AppConfig::getInstance()
{
    return *self;
}

void AppConfig::save()
{
    QSettings *configIniWrite = new QSettings(FILE_NAME_SYSCONFIG, QSettings::IniFormat);
    configIniWrite->setValue(CONFIG_MQTT_URL, mqttURL);
    configIniWrite->setValue(CONFIG_MQTT_USER, mqttUser);
    configIniWrite->setValue(CONFIG_MQTT_PASS, mqttPassword);
    configIniWrite->setValue(CONFIG_DEVICE_SN, DeviceSN);
    configIniWrite->setValue(CONFIG_SERVICE_KANG_UART, serviceModuleKangUart);
    delete configIniWrite;
}
