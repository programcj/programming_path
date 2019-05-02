/*
 * CollectCycle.cpp
 *
 *  Created on: 2015年3月19日
 *      Author: cj
 */

#include "CollectCycle.h"

//#include<time.h>
#include "../Sqlite/OrderMainOperation.h"
#include "../HAL/gpio/GPIO.h"
#include "collection.h"

//
CollectCycle CollectCycle::instance;

#define IOCTL_IO_0							0		// GPL0		DI1
#define IOCTL_IO_1							1		// GPL1		DI2
#define IOCTL_IO_2							2		// GPL2		DI3
#define IOCTL_IO_3							3		// GPL3		DI4
#define IOCTL_IO_4							4		// GPL4		DI5
#define IOCTL_IO_5							5		// GPL5		DI6

// 数字IO输入口
#define DIGITAL_IO_INPUT_0				IOCTL_IO_0
#define DIGITAL_IO_INPUT_1				IOCTL_IO_1
#define DIGITAL_IO_INPUT_2				IOCTL_IO_2
#define DIGITAL_IO_INPUT_3				IOCTL_IO_3
#define DIGITAL_IO_INPUT_4				IOCTL_IO_4
#define DIGITAL_IO_INPUT_5				IOCTL_IO_5

static PORT_COLLECT_t g_tPortCollectConfig[] ={ //采集顺序点
                {	1, DIGITAL_IO_INPUT_0, LOGIC_HIGH},
                {	2, DIGITAL_IO_INPUT_0, LOGIC_LOW},
                {	3, DIGITAL_IO_INPUT_1, LOGIC_HIGH},
                {	4, DIGITAL_IO_INPUT_1, LOGIC_LOW},
                {	5, DIGITAL_IO_INPUT_2, LOGIC_HIGH},
                {	6, DIGITAL_IO_INPUT_2, LOGIC_LOW},
                {	7, DIGITAL_IO_INPUT_3, LOGIC_HIGH},
                {	8, DIGITAL_IO_INPUT_3, LOGIC_LOW},
                {	9, DIGITAL_IO_INPUT_3, LOGIC_HIGH},
                {	10, DIGITAL_IO_INPUT_3, LOGIC_LOW}
              };

static QDateTime PortCollectStartTime[20]; //端口采集顺序 开始时间
static QDateTime CycleCollectStartTime[20]; //周期采集编号 开始时间
static QDateTime _FormingCycleStartTime;
static quint16 ModeCount = 0;

CollectCycle *CollectCycle::GetInstance()
{
    static CollectCycle* Instance = 0;
    if (0 == Instance){Instance = new CollectCycle();
    }
    return Instance;
}

void CollectCycle::interrupt(){
    isInterruptedFlag = true;
}

bool CollectCycle::isInterrupted()
{
    return isInterruptedFlag;
}

void CollectCycle::run()
{
    CycleData prodctdata;
    QDateTime curtime;
    QList<int> templist;
    int sn[20];
    isInterruptedFlag = false;

    while (!isInterrupted())
    {
        quint8 bResult;
        int ScanIndex = 0;
        memset(sn, 0, sizeof(sn));
        if (AppInfo::dev_cfg.portCollect[0] == 0)
        {
            QThread::sleep(5);
            continue;
        }

        //***************************************************
        //           依据采集顺序扫描输入信号 端口采集顺序 1 2 3 4 5 6      *
        //***************************************************
        for (; AppInfo::dev_cfg.portCollect[ScanIndex] != 0; ScanIndex++)
        {   //找到哪一个端口
            int i = 0;
            for (;	i < sizeof(g_tPortCollectConfig)
                 / sizeof(g_tPortCollectConfig[0]); i++)
            {
                if (g_tPortCollectConfig[i].index
                        == AppInfo::dev_cfg.portCollect[ScanIndex])
                {
                    break;
                }
            }
            if (i >= sizeof(g_tPortCollectConfig) / sizeof(g_tPortCollectConfig[0]))
            {
                break;
            }
            ////取端口的值 是否等于 采集值
            quint16 ucPortStatus = 0;
            do
            {
                QThread::msleep(50);
                if (!AppInfo::sys_func_cfg.isIOCollect())
                {
                    bResult = Dev_io_get_input(g_tPortCollectConfig[i].udwPortNo,
                                               ucPortStatus);
                    //qDebug("DI 端口:%d ,状态:%d\r\n", g_tPortCollectConfig[i].udwPortNo,
                    //	ucPortStatus);
                }
                else
                {
                    bResult = Rs485_collect_read(g_tPortCollectConfig[i].udwPortNo,
                                                 ucPortStatus);
                    //qDebug("collect logic:%d,485端口:%d ,状态:%d\r\n",g_tPortCollectConfig[i].udwIoLogic,g_tPortCollectConfig[i].udwPortNo, ucPortStatus);
                }

            } while (ucPortStatus != g_tPortCollectConfig[i].udwIoLogic);

            PortCollectStartTime[ScanIndex] = QDateTime::currentDateTime();
            if (ModeCount == 0 && ScanIndex == 0)
            {
                _FormingCycleStartTime = PortCollectStartTime[ScanIndex];
            }
            for (int k = 0; k < sizeof(sn) / sizeof(int); k++)
            {
                if (AppInfo::dev_cfg.CycleCollect[k * 2] == 0)
                    break;

                if (sn[k] == 0)
                {
                    // 0, 2,4 分别对应开始时间
                    if (AppInfo::dev_cfg.CycleCollect[k * 2]
                            == AppInfo::dev_cfg.portCollect[ScanIndex])
                    {
                        CycleCollectStartTime[k * 2] =
                                PortCollectStartTime[ScanIndex];
                        sn[k]++;
                    }
                }
                else if (sn[k] == 1)
                {
                    // 1, 3,5 分别对应结束时间
                    if (AppInfo::dev_cfg.CycleCollect[k * 2 + 1]
                            == AppInfo::dev_cfg.portCollect[ScanIndex])
                    {
                        CycleCollectStartTime[k * 2 + 1] =
                                PortCollectStartTime[ScanIndex];
                        sn[k]++;
                    }
                }
            }

        }

        qDebug() << "模次：" << ++ModeCount;
        quint32 machineTime = 0;
        switch (AppInfo::sys_func_cfg.machineType())
        {
        case 1:
        case 3:
        case 4:
            break;
        case 0:
        default:
        {
            //三种默认周期配置

            //机器周期
            machineTime = CycleCollectStartTime[0].msecsTo(
                        CycleCollectStartTime[1]);
            //填充周期
            prodctdata.fillTimeLong = CycleCollectStartTime[2].msecsTo(CycleCollectStartTime[3]);

            //成型周期
            prodctdata.cycleTimeLong = _FormingCycleStartTime.msecsTo(
                        CycleCollectStartTime[4]);

            //开始时间，结束时间
            prodctdata.moldstartTime = _FormingCycleStartTime;
            prodctdata.moldendTime = CycleCollectStartTime[4];
            _FormingCycleStartTime = CycleCollectStartTime[4];
            //_FormingCycleStartTime = prodctdata.moldendTime;

            //			qDebug()<<"moldstartTime" << prodctdata.moldstartTime;
            //			qDebug()<<"moldendTime" << prodctdata.moldendTime;
            //			qDebug()<<"fillTimeLong" << prodctdata.fillTimeLong;
            //			qDebug()<<"cycleTimeLong" << prodctdata.cycleTimeLong;

        }
        }

        {
            QString str1 = Tool::DateTimeToQString(prodctdata.moldendTime);
            QString str2 = Tool::DateTimeToQString(prodctdata.moldstartTime);
            QList<int> list1 = Collection::GetInstance()->read_temperature_value(
                        AppInfo::sys_func_cfg.temperateChannel());
            QList<int> list2 = Collection::GetInstance()->read_pressure_value(100);

            OrderMainOperation::GetInstance().OnAddModeCount(str1, list1,
                                                             machineTime, prodctdata.fillTimeLong, prodctdata.cycleTimeLong,
                                                             list2, str2);
        }

        QThread::msleep(100);

    }
    logDebug("thread exit.....");
}

bool CollectCycle::Dev_io_get_input(const quint8 portNo, quint16 &value)
{
    switch (portNo)
    {
    case 0:
        GPIO::getInstance().IoRead((quint16) PORT_INPUT_1, value);
        break;
    case 1:
        GPIO::getInstance().IoRead((quint16) PORT_INPUT_2, value);
        break;
    case 2:
        GPIO::getInstance().IoRead((quint16) PORT_INPUT_3, value);
        break;
    case 3:
        GPIO::getInstance().IoRead((quint16) PORT_INPUT_4, value);
        break;
    }
}

bool CollectCycle::Rs485_collect_read(const quint8 portNo, quint16 &value)
{
    if (portNo > IO_MODULE_IO_3)
    {
        return false;
    }
    return Collection::GetInstance()->getModuleChannleValue(portNo, value);

}
