#include "servicemodulekang.h"

#include "am335x_interface/interface.h"

#define  RS485Mode   1
#define  RS232Mode   0

#define B_9600_BITERATE      0
#define B_115200_BITERATE    1
#define B_1200_BITERATE      2

enum CollectChannle
{
    IO_MODULE_IO_0=0,								// IO 采集点
    IO_MODULE_IO_1=1,
    IO_MODULE_IO_2,
    IO_MODULE_IO_3,
    IO_MODULE_TEMPER_0,								// 温度采集点
    IO_MODULE_TEMPER_1,
    IO_MODULE_TEMPER_2,
    IO_MODULE_TEMPER_3,
    IO_MODULE_VOLTAGE_1,								// 电压采集点1
    IO_MODULE_VOLTAGE_2,								// 电压采集点2
    IO_MODULE_ELE_CURRENT_1,								// 电流采集点1
    IO_MODULE_ELE_CURRENT_2,								// 电流采集点2
};

static quint16 crc16(quint8 *pucBuf, quint16 uwLen)
{
    quint16 uwCRC = 0xffff;
    quint16 i, j, k;
    if (uwLen > 0)
    {
        for (i = 0; i < uwLen; i++)
        {
            uwCRC ^= pucBuf[i];
            for (j = 0; j < 8; j++)
            {
                if ((uwCRC & 0x0001) != 0)
                {
                    k = uwCRC >> 1;
                    uwCRC = k ^ 0xA001;
                }
                else
                {
                    uwCRC >>= 1;
                }
            }
        }
    }

    return uwCRC;
}

static int set_Biterate(int fd, int biteratetype)
{
    int   status;
#ifndef _WIN32
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    cfmakeraw(&Opt);
    tcflush(fd, TCIOFLUSH);

    switch(biteratetype)
    {
    case B_9600_BITERATE: //9600
        cfsetispeed(&Opt, B9600);
        cfsetospeed(&Opt, B9600);
        break;
    case B_115200_BITERATE: //115200
        cfsetispeed(&Opt, B115200);
        cfsetospeed(&Opt, B115200);
        break;
    case B_1200_BITERATE:
        cfsetispeed(&Opt, B1200);
        cfsetospeed(&Opt, B1200);
        break;
    default:
        cfsetispeed(&Opt, B9600);
        cfsetospeed(&Opt, B9600);
    }
    status = tcsetattr(fd, TCSANOW, &Opt);
    if (status != 0)
        qDebug()<<"tcsetattr fd1";
#endif
    return 0;
}

//设置参数
//simple: set_Parity(serialid, 8,1,'N');
static bool set_Parity(int fd, int databits,int stopbits,int parity)
{
#ifndef _WIN32
    struct termios options;
    if (tcgetattr( fd,&options)  !=  0)
    {
        qDebug()<<"SetupSerial 1";
        return false;
    }
    options.c_cflag &= ~CSIZE ;
    options.c_oflag = 0;
    switch (databits) /*设置数据位数*/
    {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        qDebug()<<"Unsupported data size\n";
        return false;
    }

    switch (parity)
    {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB;   /* Clear parity enable */
        options.c_iflag &= ~INPCK;     /* Enable parity checking */
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/
        options.c_iflag |= INPCK;             /* Disnable parity checking */
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;     /* Enable parity */
        options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
        options.c_iflag |= INPCK;       /* Disnable parity checking */
        break;
    case 'S':
    case 's':  /*as no parity*/
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        qDebug()<<"Unsupported parity\n";
        return false;
    }
    /* 设置停止位*/
    switch (stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        qDebug()<<"Unsupported stop bits\n";
        return false;
    }
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;

    options.c_cc[VTIME] = 0; // 1 seconds 字符之间的时间
    options.c_cc[VMIN] = 0;

    options.c_lflag &= ~(ECHO | ICANON);
    //	options.c_cflag |= CRTSCTS;

    tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        qDebug()<<"SetupSerial 3";
        return false;
    }
#endif
    return true;
}

ServiceModuleKang *ServiceModuleKang::self;

ServiceModuleKang::ServiceModuleKang(QObject *parent) :
    QThread(parent), devAddr(1), funNo(3), uwDataLen(0)
{   
    serialid=-1;
    memset(&tempqueue,0,sizeof(tempqueue));
    memset(uwIoModuleData,0,sizeof(uwIoModuleData));
    memset(revdata,0,sizeof(revdata));

    self=this;
    serialPath="/dev/ttyO2";
}

ServiceModuleKang::~ServiceModuleKang()
{
    if(serialid!=0 && serialid!=-1)
        close(serialid);
}

ServiceModuleKang *ServiceModuleKang::getInstance()
{
    return self;
}

void ServiceModuleKang::setUart(const QString &path)
{
    QMutexLocker lock(&mutex);
    serialPath=path;
    statusMssage="设定串口:"+serialPath;
    qDebug()<<statusMssage;
}

bool ServiceModuleKang::getModuleChannleValue(quint8 channlenum, int *value)
{
    if (channlenum < IO_MODULE_CHANNEL_NUM)
    {
        QMutexLocker lock(&mutex);
        *value = uwIoModuleData[channlenum];
        return true;
    }
    return false;
}

QList<int> ServiceModuleKang::read_temperature_value(int tempNum)
{
#define DEFALUT_TEMPERATAUTE_VALUE 1000
    QList<int> tp_data;
    for (int i = 0; i < tempNum; i++)
    {
        if (i < 4)
        {
            if(tempqueue[i].count > 10)
            {
                tempqueue[i].lasttemp = (tempqueue[i].sum - (tempqueue[i].maxtemp + tempqueue[i].mintemp))/(tempqueue[i].count-2);
                tempqueue[i].count = 0;
                tempqueue[i].sum = 0;
                tempqueue[i].maxtemp = tempqueue[i].lasttemp;
                tempqueue[i].mintemp = tempqueue[i].lasttemp;
            }
            tp_data.append(tempqueue[i].lasttemp);
            //			qDebug() << "IO_MODULE_TEMPER_channel" << IO_MODULE_TEMPER_0 + i
            //					<< tempqueue[i].lasttemp;
        }
        else
        {
            tp_data.append(DEFALUT_TEMPERATAUTE_VALUE);
        }
    }

    return tp_data;
}

QList<int> ServiceModuleKang::read_pressure_value(int pointNum)
{
    QList<int> tp_data;
    tp_data = pressureList.mid(0,
                               pressureList.size() > pointNum ?
                                   pointNum - 1 : pressureList.size());
    if (pressureList.size() > pointNum)
    {
        pressureList = pressureList.mid(pointNum, pressureList.size());
    }
    else
    {
        pressureList.clear();
    }
    return tp_data;
}

bool ServiceModuleKang::setTempCalibre(int channel, int value)
{
    quint8 ucBuf[MAX_MODBUS_FRAME_LEN];
    quint8 ucCnt = 0;
    quint16 uwCheckSum = 0;

    // 地址1B
    ucBuf[ucCnt++] = IO_DEVICE_ADDR_KND;
    // 功能码1B
    ucBuf[ucCnt++] = FN_WRITE_REG;
    // 起始地址2B
    ucBuf[ucCnt++] = (quint8)((IO_REG_ADDR_CAB_TEMPER_0 + channel) >> 8);
    ucBuf[ucCnt++] = (quint8)(IO_REG_ADDR_CAB_TEMPER_0 + channel);
    // 寄存器个数2B
    ucBuf[ucCnt++] = (quint8)(1 >> 8);
    ucBuf[ucCnt++] = (quint8)(1);
    // 数据长度1B
    ucBuf[ucCnt++] = 2;
    // 数据,寄存器个数×2字节，每个数据高字节在前2B*RegNum
    //数据内容
    ucBuf[ucCnt++] = (quint8)(value >> 8);
    ucBuf[ucCnt++] = (quint8)(value);

    // CRC16    2B
    uwCheckSum = crc16(ucBuf, ucCnt);
    ucBuf[ucCnt++] = (quint8)(uwCheckSum);
    ucBuf[ucCnt++] = (quint8)(uwCheckSum >> 8);

    if (sendDataToComm(ucBuf, ucCnt) == ucCnt)
    {
        return true;
    }
    qDebug()<<"sendDataToComm failed!";
    return false;
}

void ServiceModuleKang::recvDump(const unsigned char *dataBuffer, int dataLen)
{
    dataBuffer=NULL;
    dataLen=0;
}

/**
 * 解码出DI的值，温度
 * @brief ServiceModuleKang::parsedata
 * @param orgdata
 * @param len
 */
void ServiceModuleKang::parsedata(unsigned char *orgdata, int len)
{
    quint16 udwFlag = 0x0F80;
    qDebug()<<"parse > IO_MODULE_CHANNEL_NUM="<<IO_MODULE_CHANNEL_NUM;

    for (int i = 0; i < IO_MODULE_CHANNEL_NUM; i++)
    {
        uwIoModuleData[i] = (orgdata[i * 2 + 0] << 8) + orgdata[i * 2 + 1];
        if (i < 4)
        {
            if ((uwIoModuleData[i] & udwFlag) == udwFlag)
                uwIoModuleData[i] = 1;
            else
                uwIoModuleData[i] = 0;
            qDebug()<<"ModuleData:"<<i<<uwIoModuleData[i];
        }
        else if (i == IO_MODULE_VOLTAGE_1 || i == IO_MODULE_VOLTAGE_2)
        {
            pressureList.append(uwIoModuleData[i]);
        }
        else if(i >= IO_MODULE_TEMPER_0 && i<= IO_MODULE_TEMPER_3 )
        {
            int j =i - IO_MODULE_TEMPER_0;
            tempqueue[j].tempbuff[ tempqueue[j].count++] = uwIoModuleData[i];
            if(uwIoModuleData[i] >	tempqueue[j].maxtemp)
                tempqueue[j].maxtemp = uwIoModuleData[i];
            if(uwIoModuleData[i] <	tempqueue[j].mintemp)
                tempqueue[j].mintemp = uwIoModuleData[i];
            tempqueue[j].sum +=uwIoModuleData[i];
            qDebug()<<"tempvaule:"<<j<<uwIoModuleData[i];
        }
        qDebug()<<"ModuleData"<<i<<uwIoModuleData[i];
    }
    len=0;
}

int ServiceModuleKang::sendDataToComm(unsigned char *data, quint16 len)
{
    QMutexLocker lock(&mutex);

    if(serialid==0 || serialid==-1) {
        qDebug() << "not send data";
        return -1;
    }

    //如果是485就需要控制管脚

    int ret=write(serialid,data, len);
    tcdrain(serialid);
    qDebug() << "Send Lenght:" <<ret<<",need send:" << len;
    return ret;
}

bool ServiceModuleKang::SendCmdToComm(quint8 ucDevAddr, quint8 ucFunNo, quint16 uwStartAddr, quint16 uwData)
{
    quint8 ucBuf[1024];
    int ucCnt = 0;
    quint16 uwCheckSum = 0;

    // 4字节长度的总线空闲时间
    // 地址1B
    ucBuf[ucCnt++] = ucDevAddr;
    // 功能码1B
    ucBuf[ucCnt++] = ucFunNo;
    // 起始地址	2B
    ucBuf[ucCnt++] = (quint8) (uwStartAddr >> 8);
    ucBuf[ucCnt++] = (quint8) (uwStartAddr);
    // 寄存器个数2B
    ucBuf[ucCnt++] = (quint8) (uwData >> 8);
    ucBuf[ucCnt++] = (quint8) (uwData);
    // CRC16  2B
    uwCheckSum = crc16(ucBuf, ucCnt);
    ucBuf[ucCnt++] = (quint8) (uwCheckSum);
    ucBuf[ucCnt++] = (quint8) (uwCheckSum >> 8);

    // 4字节长度的总线空闲时间
    if (sendDataToComm(ucBuf, ucCnt) != ucCnt)
    {
        qDebug() << "sendDataToComm failed!";
        return false;
    }
    return true;
}

bool ServiceModuleKang::set_answer_delay(quint16 puwData)
{
    quint8 ucBuf[MAX_MODBUS_FRAME_LEN];
    //quint8 i = 0;
    quint8 ucCnt = 0;
    quint16 uwCheckSum = 0;

    // 4字节长度的总线空闲时间

    // 地址1B
    ucBuf[ucCnt++] = IO_DEVICE_ADDR_KND;
    // 功能码1B
    ucBuf[ucCnt++] = FN_WRITE_REG;
    // 起始地址2B
    ucBuf[ucCnt++] = (quint8) (0x0011 >> 8);
    ucBuf[ucCnt++] = (quint8) (0x0011);
    // 寄存器个数2B
    ucBuf[ucCnt++] = (quint8) (1 >> 8);
    ucBuf[ucCnt++] = (quint8) (1);
    // 数据长度1B
    ucBuf[ucCnt++] = 1 * 2;
    // 数据,寄存器个数×2字节，每个数据高字节在前2B*RegNum

    ucBuf[ucCnt++] = (quint8) (puwData >> 8);
    ucBuf[ucCnt++] = (quint8) (puwData);

    // CRC16    2B
    uwCheckSum = crc16(ucBuf, ucCnt);
    ucBuf[ucCnt++] = (quint8) (uwCheckSum);
    ucBuf[ucCnt++] = (quint8) (uwCheckSum >> 8);
    // 4字节长度的总线空闲时间

    /*if(uart_write(MODBUS_PORT, ucBuf, ucCnt) != ucCnt)
     {
     return FALSE;
     }*/
    //mutex.lock();
    if (sendDataToComm(ucBuf, ucCnt) != ucCnt)
    {
        qDebug() << "sendDataToComm failed!";
        //mutex.unlock();
        return false;
    }
    //mutex.unlock();
    return true;
}

void ServiceModuleKang::onDataProtocol(unsigned char *data, int len)
{
    quint16 uwCalcCheckSum = 0;
    quint16 uwGetCalcCheckSum = 0;
    quint16 datalen = 0;
    quint8 tempdata[1024];

    if ((data[0] == devAddr) && (data[1] == funNo))
    {
        memcpy(revdata, data, len);
        uwDataLen = len;
    } else if (uwDataLen >= 0 && uwDataLen < MAX_MODBUS_FRAME_LEN) {
        memcpy(revdata + uwDataLen, data, len);
        uwDataLen += len;
    } else {
        qDebug() << "iomodule data error!" << "LEN:" << len << "uwDataLen:"
                 << uwDataLen;
        uwDataLen = 0;
    }

    while (1)
    {
        for (int i = 0; i < uwDataLen; i++)
        {
            if (revdata[i] == devAddr && revdata[i + 1] == funNo)
            {
                if (i > 0)
                    memcpy(revdata, revdata + i, uwDataLen - i);
                uwDataLen -= i;
                if (uwDataLen > 2)
                    datalen = revdata[2];  	       //数据的长度
                break;
            }
        }
        if (uwDataLen <= datalen || datalen == 0)
        {
            break;
        }
        uwCalcCheckSum = crc16(revdata, datalen + 3);  //计算checksum值
        uwGetCalcCheckSum = (revdata[datalen + 3 + 1] << 8)
                + revdata[datalen + 3 + 0]; //取checksum 的值

        if (uwCalcCheckSum == uwGetCalcCheckSum)
        {
            memcpy(tempdata, (revdata + 3), datalen);

            parsedata(tempdata, datalen);

            if (uwDataLen > datalen + 5)
            {
                uwDataLen = uwDataLen - datalen - 5;
                memcpy(revdata, (revdata + datalen + 5), uwDataLen);
            }
            else
            {
                uwDataLen = 0;
                break;
            }

            if (uwDataLen < 29)
            {
                break;
            }
        }
        else
        {
            qDebug() << "checksum error!" << "data length:" << uwDataLen;
            break;
        }
    }
}

#define max_recv_buffer  (4*1024)

void ServiceModuleKang::run()
{
    unsigned char recvBuffer[max_recv_buffer];
    int revlen = 0;
    int len = 0;
    int time = 0;  //接收次数
    quint16 delaytime = 20;

    QByteArray path=serialPath.toLatin1();

    this->mutex.lock();
    statusMssage=QString("运行开始:%1").append(serialPath);

    serialid = open(path.data(),O_RDWR | O_NOCTTY | O_NONBLOCK);
    //O_RDWR | O_NOCTTY | O_NONBLOCK);
    //O_RDWR | O_NOCTTY | O_NDELAY);
    if(serialid==-1){
        qDebug() << "serial Open err->"<<serialPath;
        this->mutex.unlock();
        statusMssage=QString("运行出错:串口打开错误:%1").arg(serialPath);
        return;
    }

    statusMssage="运行成功:"+serialPath;
    set_Biterate(serialid, B_115200_BITERATE);
    set_Parity(serialid, 8,1,'N');
    this->mutex.unlock();

    qDebug() <<"ServiceModuleKang run..:" <<serialid;
    set_answer_delay(delaytime);

    while (!isInterruptionRequested())
    {
#ifndef _WIN32
        if (SendCmdToComm(IO_DEVICE_ADDR_KND, FN_ART_READ_KEEP_CHANNEL, 0x0200,	12))
        {
            len = 0;
            revlen = 0;
            time = 0;
            while (time++ < 10 && !isInterruptionRequested())
            {
                QThread::msleep(10);
                len = read(serialid, recvBuffer + revlen, max_recv_buffer);
                if (len > 0)
                {
                    revlen += len;
                    if (revlen >= 29)
                        break;
                }
                else
                {
                    if(len == -1)
                        qDebug() <<" 485  error -1!";
                }
                QThread::msleep(10);
            }
            if (revlen > 0)
            {
                qDebug("------>serialdataReady:%d\n",revlen);
                for(int i=0;i<revlen;i++){
                    fprintf(stderr,"%02X ", (unsigned char)recvBuffer[i]);
                }
                qDebug("=================================\n");
                onDataProtocol(recvBuffer, revlen);
            }
            else
            {
                qDebug(" 485 revice error! revlen =%d", revlen);
            }
        } else {
            qDebug() << "485 send error";
        }
        //        if(DigitalMeter->getReadMeterFlag())
        //        {
        //            quint32 masterdata;
        //            DigitalMeter->setSerialHandle(serial);
        //            DigitalMeter->DigitalMeter_send_read_ele_frame();
        //            DigitalMeter ->DigitalMeter_read_data(&masterdata);
        //            DigitalMeter ->saveMasterData(masterdata);
        //            //DigitalMeter->setReadMeterFlag(false);
        //            serial->init(B_115200_BITERATE,8,1,'N');
        //        }
#endif
        QThread::msleep(100);
    }
    this->mutex.lock();
    close(serialid);
    serialid=0;
    statusMssage=QString("已关闭:%1").arg(serialPath);
    this->mutex.unlock();
    qDebug()<<statusMssage;
}
