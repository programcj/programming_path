#ifndef SERVICEMODULEKANG_H
#define SERVICEMODULEKANG_H

#include <QObject>
#include <QMutexLocker>
#include <QDebug>
#include <QThread>

#define IO_MODULE_CHANNEL_NUM				  	12		// IO模块通道数量#define IO_MODULE_SIM_CHANNEL_NUM			   	6		// 模拟量通道数// MODBUS最大数据帧长度
#define MAX_MODBUS_FRAME_LEN				       1024

//功能码
#define FN_ART_READ_KEEP_CHANNEL			0x03    //标准读功能寄存器
#define FN_ART_READ_INPUT_CHANNEL			0x04   // 读输入寄存器    //  功能码


//****************** MODBUS 供能码 ***********************//
#define FN_READ_CHANNEL						FN_ART_READ_INPUT_CHANNEL
#define FN_WRITE_REG						0x10
//宏定义
#define THREAD_COLLECTED_ENABLE			 	1		// 1 = 使能线程采集		0 = 关闭
#define TEMPERATURE_SAVED_MAX				25
#define USE_ELE_CURRENT					0		// 1 = 压力电流     0 = 压力电压
//各模块地址
#define IO_DEVICE_ADDR_KND					1  // 康耐德 IO
#define IO_DEVICE_ADDR_ART_3038					2  // ART 3038
#define IO_DEVICE_ADDR_ART_3054					3  // ART 3054

//输入寄存器起始地址
#define IO_MODULE_START_ADDR_KND			0x0200
#define IO_MODULE_START_ADDR_ART			257   //0x7631 //40288// 十进制 30257


#define IO_REG_ADDR_CAB_TEMPER_0		0x020C
#define IO_REG_ADDR_CAB_TEMPER_1		0x020D
#define IO_REG_ADDR_CAB_TEMPER_2		0x020E
#define IO_REG_ADDR_CAB_TEMPER_3		0x020F

//温度
struct  Temperature
{
    quint16 tempbuff[1024];
    quint16 count;
    quint16 maxtemp;
    quint16 mintemp;
    quint16 lasttemp;
    quint32 sum;
};

class ServiceModuleKang : public QThread
{
    static ServiceModuleKang *self;
    QString statusMssage;

    Q_OBJECT
public:
    explicit ServiceModuleKang(QObject *parent = 0);

    ~ServiceModuleKang();

    static ServiceModuleKang *getInstance();

    void setUart(const QString &path);

    //12个IO 口数据
    bool getModuleChannleValue(quint8 channlenum,int *value);

    //读取温度
    QList<int> read_temperature_value(int tempNum);

    //读取压力
    QList<int> read_pressure_value(int pointNum);

    //温度校准
    bool setTempCalibre(int channel,int value);

    QString getStatusMessage(){
        return statusMssage;
    }

private:
    int serialid;
    QString serialPath;
    QMutex mutex;

    quint8 revdata[MAX_MODBUS_FRAME_LEN];
    quint8 devAddr;
    quint8 funNo;
    quint16 uwDataLen;

    void recvDump(const unsigned char* dataBuffer, int dataLen);

    void parsedata(unsigned char *orgdata, int len);

    //IO 模型数据 DI1-DI4, A1-A4
    quint16 uwIoModuleData[IO_MODULE_CHANNEL_NUM];

    //压力
    QList<int> pressureList;

    //8温度
    Temperature tempqueue[8];

    int sendDataToComm(unsigned char* data,quint16 len);

    bool SendCmdToComm(quint8 ucDevAddr, quint8 ucFunNo, quint16 uwStartAddr, quint16 uwData);

    //设置485响应时间
    bool set_answer_delay(quint16 puwData);

protected:
    void onDataProtocol(unsigned char* data,int len);

    // 线程执行体
    virtual void run();
};

#endif // SERVICEMODULEKANG_H
