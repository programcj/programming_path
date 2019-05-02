/*
 * public.h
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#ifndef PUBLIC_H_
#define PUBLIC_H_

#include <QApplication>

#include "../Unit/MESLog.h"
#include "../Unit/Tool.h"

#include "../Sqlite/SQLiteBaseHelper.h"
#include "../Sqlite/SQLiteProductedHelper.h"
#include "../Sqlite/Entity/Employee.h"
#include "../Sqlite/Entity/Material.h"
#include "../Sqlite/Entity/Notebook.h"
#include "../Sqlite/Entity/Order.h"
#include "../Sqlite/Entity/Producted.h"
#include "../Sqlite/OrderMainOperation.h"

#include "../NetCSCP/CSCPNotebookTask.h"
#include "../AppConfig/AppInfo.h"
#include "../Server/CollectCycle.h"
#include "../Action/plugin/mesmessagedlg.h"
#include "../HAL/gpio/GPIO.h"
#include "appversion.h"
#include "ipcmessage.h"

using namespace unit;
using namespace sqlite;
using namespace entity;


//软件版本设置 方法     X - y - Z - v
//                 x:硬件、软件版本  （1003）
//                 y： 客户版本 （3001-3099）
//                 z：软件更新周期（10-199）a 表测试版本
//                 v: SVN 版本号
#define APP_VER_STR      "1.3.10."APPVERSION


#define IO_MODULE_CHANNEL_NUM				  	12		// IO模块通道数量#define IO_MODULE_SIM_CHANNEL_NUM			   	6		// 模拟量通道数// MODBUS最大数据帧长度
#define MAX_MODBUS_FRAME_LEN				       1024

typedef struct
{
  quint8 index;                                         // 采集编号
  quint8 udwPortNo;                                     // 端口号
  quint8 udwIoLogic;                                    // 端口状态

}PORT_COLLECT_t;

#define SYS_SIGNAL_TYPE_DI              0 //采集器DI采集
#define SYS_SIGNAL_TYPE_IO              1 //IO模块DI采集

#endif /* PUBLIC_H_ */
