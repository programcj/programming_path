#ifndef PUBLIC_H
#define PUBLIC_H

#include <qglobal.h>

#ifdef Q_OS_OSX

#endif
#ifdef Q_OS_LINUX
//#define AM335X
#define Q_OS_LINUX_WEIQIAN
#endif

#define FILE_NAME_DATABASE      "hmi-database.db"
#define FILE_NAME_SYSCONFIG     "hmi-config.ini"
#define FILE_NAME_FUNCARD       "hmi-funcard.json"
#define FILE_NAME_DICYCLE_CONFIG       "hmi-config.ServiceDICycle.json"

//
#if QT_VERSION > 0x050000

#include "qjsondocument.h"
#include "qjsonobject.h"

#else  //0x040805

#include "tools/qjson-backport-master/qjsondocument.h"
#include "tools/qjson-backport-master/qjsonobject.h"
#include "tools/qjson-backport-master/qjsonarray.h"

#endif

#endif // PUBLIC_H

