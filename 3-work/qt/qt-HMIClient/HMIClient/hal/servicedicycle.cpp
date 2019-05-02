#include "servicedicycle.h"
#include "am335x_interface/interface.h"
#include "am335x_interface/gpio_test.h"
#include "servicemodulekang.h"
#include "../dao/mesdispatchorder.h"
#include "../dao/mesproducteds.h"
#include "../dao/brushcard.h"
#include "../tools/mestools.h"
#include "../application.h"

ServiceDICycle *ServiceDICycle::self;
//输入信号：
//	4路输入 (开关量)
//	相关的设备接口文件 ：
//		DI1 ->  /sys/class/gpio/gpio113/value
//		DI2 ->  /sys/class/gpio/gpio111/value
//		DI3 ->  /sys/class/gpio/gpio116/value
//		DI4 ->  /sys/class/gpio/gpio117/value
//-////////////////////////////////////////////////////////////////////////

void ServiceDICycle::saveConfigInfo(QJsonObject &json)
{
    QFile file(FILE_NAME_DICYCLE_CONFIG);

    if(file.open(QIODevice::ReadWrite | QIODevice::Text )) {
        QString text=QString(QJsonDocument(json).toJson());
        QTextStream in(&file);
        in << text;
        in.flush();
        file.close();
    }
    if(self!=NULL){
        self->loadConfigInfo();
    }
}

ServiceDICycle::ServiceDICycle(QObject *parent) :
    QThread(parent)
{
#if QT_VERSION < 0x050000
    interruptionRequested=false;
#endif
    self=this;
    memset(lineA,0,sizeof(lineA));
    memset(lineB,0,sizeof(lineB));
    memset(lineC,0,sizeof(lineC));
}

ServiceDICycle *ServiceDICycle::getInstance()
{
    return self;
}

void ServiceDICycle::loadConfigInfo()
{
    QJsonObject jsonConfig;
    QFile file(FILE_NAME_DICYCLE_CONFIG);

    if(!file.exists()){
        file.open(QIODevice::ReadWrite | QIODevice::Text);
        return;
    }

    //    {
    //      "FDAT_ConfigType": 1,
    //      "FDAT_ConfigData": { -----<<<<<
    //        "Default": "ScanConfig-GPIO",
    //        "ScanConfig-Kang": {
    //          "Type":"COM",
    //          "COM":"232",
    //          "LineA": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "Port": 1,
    //              "Value": 0
    //            }
    //          },
    //          "LineB": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "GPIO": 1,
    //              "Value": 0
    //            }
    //          },
    //          "LineC": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "GPIO": 1,
    //              "Value": 0
    //            }
    //          }
    //        },
    //        "ScanConfig-GPIO": {
    //          "DefaultValue": 1,
    //          "Type":"GPIO",
    //          "GPIOMax": 3,
    //          "GPIOMin": 0,
    //          "LineA": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "Port": 1,
    //              "Value": 0
    //            }
    //          },
    //          "LineB": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "GPIO": 1,
    //              "Value": 0
    //            }
    //          },
    //          "LineC": {
    //            "StartPort": {
    //              "GPIO": 1,
    //              "value": 1
    //            },
    //            "EndPort": {
    //              "GPIO": 1,
    //              "Value": 0
    //            }
    //          }
    //        }
    //      }
    //    }
    if(file.open(QIODevice::ReadWrite | QIODevice::Text )) {
        QTextStream in(&file);
        QString text=in.readAll();
        file.close();

        QJsonParseError json_error;
        QJsonDocument parse_doucment = QJsonDocument::fromJson(text.toUtf8(), &json_error);
        jsonConfig=parse_doucment.object();

        this->scanModule=jsonConfig["Default"].toString();
        if(jsonConfig.contains(scanModule)) {
            QJsonObject scanConfigJson=jsonConfig[scanModule].toObject();

            QJsonObject JsonlineA=scanConfigJson["LineA"].toObject();
            QJsonObject JsonlineB=scanConfigJson["LineB"].toObject();
            QJsonObject JsonlineC=scanConfigJson["LineC"].toObject();

            QJsonObject port=JsonlineA["StartPort"].toObject();
            this->lineA[0]=port["GPIO"].toInt();
            this->lineA[1]=port["value"].toInt();

            port=JsonlineA["EndPort"].toObject();
            this->lineA[2]=port["GPIO"].toInt();
            this->lineA[3]=port["value"].toInt();
            //------------------------------------------
            port=JsonlineB["StartPort"].toObject();
            this->lineB[0]=port["GPIO"].toInt();
            this->lineB[1]=port["value"].toInt();
            port=JsonlineB["EndPort"].toObject();
            this->lineB[2]=port["GPIO"].toInt();
            this->lineB[3]=port["value"].toInt();
            //------------------------------------------
            port=JsonlineC["StartPort"].toObject();
            this->lineC[0]=port["GPIO"].toInt();
            this->lineC[1]=port["value"].toInt();
            port=JsonlineC["EndPort"].toObject();
            this->lineC[2]=port["GPIO"].toInt();
            this->lineC[3]=port["value"].toInt();
        }
    }
}

#if QT_VERSION < 0x050000
void ServiceDICycle::requestInterruption()
{
    QMutexLocker locker(&selfMutex);
    interruptionRequested=true;
}

bool ServiceDICycle::isInterruptionRequested()
{
    QMutexLocker locker(&selfMutex);
    return interruptionRequested;
}
#endif

void ServiceDICycle::readLine(int gpio, int *value)
{
    //line: 1 2 3 4
#ifdef AM335X
    *value=-1;

    if(scanModule=="ScanConfig-GPIO"){
        if(0!=gpio_input_read(gpio, value)){
            *value=-1;
            qDebug() << "gpio error ......"<<gpio;
        } else {
            led_set(gpio,*value);//0 1 2 3; 4 WIFI
        }
        return;
    }

    if(scanModule=="ScanConfig-Kang"){
        if(ServiceModuleKang::getInstance()->getModuleChannleValue(gpio, value)){

        } else {
            qDebug() << "ServiceModuleKang gpio error ......"<<gpio;
        }
    }

#endif
}

void ServiceDICycle::onCountAdd(QDateTime &lineAstart,
                                QDateTime &lineAnextStart,
                                QDateTime &lineBstart,
                                QDateTime &lineBend,
                                QDateTime &lineCend)
{
    qint64  endmsc=lineAstart.msecsTo(lineAnextStart);  //成型周期
    qint64  ooomsc=lineBstart.msecsTo(lineBend);   //射胶周期(填充周期)
    qint64  machmsc=lineAstart.msecsTo(lineCend);   //机器周期
    //------------------------------------------------
    //    切换到模次
    //    派工单号
    //    生产数据开始时间（成型周期开始时间）
    //    生产数据产生时间（成型周期结束时间）
    //    温度路数(N)
    //    4路温度值，每路2字节依序为
    //    温度1 （2字节）
    //    温度2 （2字节）
    //    温度3 （2字节）
    //    温度4 （2字节）
    //    ....温度N （逗号隔开）
    //    机器周期，毫秒
    //    填充时间，毫秒
    //    成型周期，毫秒
    //    模次（成模次数，啤数）
    //    压力点个数（N）
    //    100个压力点（逗号隔开）
    //-----------------------------------------------------------
    //-温度值:从温度缓存里面获取4路的最新一个
    //-压力点: 从压力点缓存里面获取最新100个
    //-----------------------------------------------------------
    // 如果有停机卡，就不计算模次
    // 获取主派工单号，为此派工单的模次增加数量
    // 每一件的显示呢? 也须要增加
    //    件的模穴数*模次＝生产数(服务器会记录，与实际有差别不管)
    // 判断 达到生产数是否自动换单
    // 产生生产数据并提交到生产数据库
    //-----------------------------------------------------------
    QList<int> temper;
    QList<int>  keepPress;
    {
        QString mainDispatchNo=MESDispatchOrder::onGetMainDispatchNo();
        QJsonObject json;

        if(Application::getInstance()->haveStopCardFlag){
            return;
        }

        MESDispatchOrder::onAddMOTotalNum(mainDispatchNo,1);

        int totalNumber=MESDispatchOrder::onGetDispatchNoMoTotalNum(mainDispatchNo);

        json.insert("PD_DispatchNo",mainDispatchNo);  //派工单号
        json.insert("PD_StartCycle",MESTools::DateTimeToQString(lineAstart));
        json.insert("PD_EndCycle", MESTools::DateTimeToQString(lineAnextStart));
        json.insert("PD_TemperNum",4); //温度路数(N)
        QJsonArray temperValues; //4路温度值，每路2字节依序为
        {
            QList<int> tempList=ServiceModuleKang::getInstance()->read_temperature_value(4);
            for(int i=0,j=tempList.size();i<j;i++){
                temperValues.append(tempList[i]);
            }
        }
        json.insert("PD_TemperValue", temperValues);
        json.insert("PD_MachineCycle",machmsc);//机器周期，毫秒
        json.insert("PD_FillTime", ooomsc); //填充时间，毫秒
        json.insert("PD_CycleTime", endmsc);// 成型周期，毫秒
        json.insert("PD_TotalNum", totalNumber );// 模次（成模次数，啤数）
        json.insert("PD_KeepPressNum",100);

        QJsonArray PD_KeepPressNum;
        {
            QList<int> press=ServiceModuleKang::getInstance()->read_pressure_value(100);
            for(int i=0,j=press.size();i<j;i++){
                PD_KeepPressNum.append(press[i]);
            }
        }
        json.insert("PD_KeepPress", PD_KeepPressNum);

        MESProducteds::onAppendMsg(0x11, json);

        if(true==MESDispatchOrder::onCheckProductedNumberEnd(mainDispatchNo)) {
            //auto change Main order
            //判断 达到生产数是否自动换单 //比较生产数
            // OnReplaceOrder(); //自动换单
            //OnUpdate();
            //            BrushCard brush(mainOrderCache, "00000001", BrushCard::AsCHANGE_ORDER,
            //                    Tool::GetCurrentDateTimeStr(), 0);
            //            Notebook("自动换单开始", brush).insert();
            //            brush.setIsBeginEnd(1);
            //            Notebook("自动换单结束", brush).insert();

            MESDispatchOrder::onDeleteDispatchNo(mainDispatchNo);

            BrushCard bushCard;
            bushCard.CC_DispatchNo=mainDispatchNo;
            bushCard.CC_CardID ="自动换单";
            bushCard.CC_CardType= 6;
            bushCard.CC_CardData=MESTools::GetCurrentDateTimeStr();
            bushCard.CC_IsBeginEnd=0;
            bushCard.Name="自动换单-开始";

            QJsonObject json=bushCard.toJsonObject();
            //QString submitMsg=QString(QJsonDocument(json).toJson());

            MESProducteds::onAppendMsg(0x12, json);

            //MESMqttClient::getInstance()->sendD2S(0x12, 00, submitMsg);
            bushCard.CC_DispatchNo=MESDispatchOrder::onGetMainDispatchNo();
            bushCard.CC_IsBeginEnd=1;
            bushCard.Name="自动换单到下一单,这里上传新主单";
            json=bushCard.toJsonObject();
            MESProducteds::onAppendMsg(0x12, json);
        }
        emit sigTotalNumAdd(totalNumber,endmsc);
    }
}



void ServiceDICycle::run()
{
    //采集计数功能

    loadConfigInfo();

#ifdef AM335X
    gpio_init_input();
#endif

    //    int portCollect[20];  //端口采集顺序：
    //    int CycleCollect[20];	//周期采集编号：20字节，两个字节为一组周期的起始和结束编号，共可填入10组采集周期
    //    int FunCollectEnable[20];  //功能采集使能：20字节，8字节对应IO模块每个端口的状态，0为关闭，其他为使能。其他字节保留。
    //    int i=0;
    //----------------------------------------------------------------------------------------
    //	目前增加三个周期：
    //i.	机器周期（2字节）The machine cycle   start[0]  end[1];
    //ii.	填充周期（2字节） Fill      start[2]   end[3]
    //iii.	成型周期（2字节）Forming    start[4]   end[5]
    //iv.	其他保留（14字节）Other
    //-----------------------------------------------------------------------------------------
    //   A线:[起始信号:端口:值][结束信号:端口:值]
    //   B线:[起始信号:端口:值][结束信号:端口:值] 射胶周期(填充周期)
    //   C线:[起始信号:端口:值][结束信号:端口:值]
    //-----------------------------------------------------------------------------------------
    //    A开始－>C结束:(机器周期)
    //    A开始->C线结束->A开始:(成型周期)一次生成数据
    //-----------------------------------------------------------------------------------------
    // GPIO_INPUT:0,1,2,3,默认高电平
    //-----------------------------------------------------------------------------------------
    //int lineA[4]={1,0,1,1};
    //int lineB[4]={2,1,2,0};
    //int lineC[4]={3,1,3,0};
    /////////////////////////////////////
    QDateTime timeLine[6];
    QDateTime timeFirstStart;
    bool first=true;

    //line A: t[0]    t[1]
    //line B:                 t[2]      t[3]
    //line C                                  t[4]     t[5]
    //-------------------------------------------------------------
    while(!isInterruptionRequested()) {
        int value=0;
#ifndef AM335X
        do{
            timeFirstStart=QDateTime::currentDateTime();
            QThread::sleep(1);
            timeLine[2]=QDateTime::currentDateTime();
            QThread::sleep(2);
            timeLine[3]=QDateTime::currentDateTime();
            QThread::sleep(6);
            timeLine[5]=QDateTime::currentDateTime();
            timeLine[0]=QDateTime::currentDateTime();
            onCountAdd(timeFirstStart, timeLine[0], timeLine[2], timeLine[3], timeLine[5]);
        }while(!isInterruptionRequested());
#endif
        qDebug() << "------------------------------------- read line A -----------------------------------------------";
        while(!isInterruptionRequested()) {
            readLine(lineA[0], &value);
            if(value==lineA[1]) { //lineA start
                timeLine[0]=QDateTime::currentDateTime();
                if(!first) { ///第一次开始标志，false才开始
                    /////一次生成数据
                    /////  timeLine[0]-timeFirstStart = 成型周期
                    /////  timeLine[3]-timeLine[2]  射胶周期(填充周期)
                    /////  timeFirstStart-timeLine[5]   机器周期
                    /////  timeFirstStart
                    /// 生产数据开始时间（成型周期开始时间）= timeFirstStart
                    /// 生产数据产生时间（成型周期结束时间）= timeLine[0]
                    /// ------------------------------
                    /// 温度实时读(有4路温度，每一路取一个上报,温度采集线程缓存中取)：
                    /// ------------------------------
                    /// 压力点,   射胶周期/100=每个点的取值间隔,异步获取
                    ///
                    ///                    qint64  endmsc=timeFirstStart.msecsTo(timeLine[0]);
                    ///                    qint64  ooomsc=timeLine[2].msecsTo(timeLine[3]);
                    ///                    qint64  machmsc=timeFirstStart.msecsTo(timeLine[5]);
                    onCountAdd(timeFirstStart, timeLine[0], timeLine[2], timeLine[3], timeLine[5]);
                }
                if(first==true){
                    first=false;
                }
                timeFirstStart = QDateTime::currentDateTime();
                break;
            }
        }

        while(!isInterruptionRequested()) {
            readLine(lineA[2], &value);
            if(value==lineA[3]) { //lineA start
                timeLine[1]=QDateTime::currentDateTime();
                break;
            }
        }
        //timeLine[0]= 成型周期开始
        qDebug() << "------------------------------------- read line B -----------------------------------------------";
        while(!isInterruptionRequested()) {
            readLine(lineB[0], &value);
            if(value==lineB[1]) { //lineA start
                timeLine[2]=QDateTime::currentDateTime();
                break;
            }
        }

        while(!isInterruptionRequested()) {
            readLine(lineB[2], &value);
            if(value==lineB[3]) { //lineA start
                timeLine[3]=QDateTime::currentDateTime();
                break;
            }
        }
        /// timeLine[3]- timeLine[2] => 射胶周期(填充周期)
        //////////////////////////////////////////////////////////// line C //////////////////////////////////////
        qDebug() << "------------------------------------- read line C -----------------------------------------------";
        while(!isInterruptionRequested()){
            readLine(lineC[0], &value);
            if(value==lineC[1]) { //lineA start
                timeLine[4]=QDateTime::currentDateTime();
                break;
            }
        }

        while(!isInterruptionRequested()) {
            readLine(lineC[2], &value);
            if(value==lineC[3]) { //lineA start
                timeLine[5]=QDateTime::currentDateTime();
                break;
            }
        }
        ////////////////////////////////////////////////////////////--end--- //////////////////////////////////////
    }
    qDebug() << "Thread exit**************";
}
