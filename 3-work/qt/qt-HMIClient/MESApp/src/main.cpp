//#include "UnitTest/serialtest.h"
#include "Server/rfserver.h"
#include "Server/collection.h"
#include "Action/mainwindow.h"
#include "Action/inputpannelcontext.h"
#include <QNetworkInterface>
#include "Server/NetCommandService.h"
#include "application.h"
#ifdef  Q_OS_LINUX   //for linux
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

class MYTest: public QObject
{
public:
	void start()
	{
		startTimer(20000);
	}

	virtual void timerEvent(QTimerEvent *)
	{
		QString endCycle; //生产数据产生时间
		QList<int> temperList;	//N路温度 当前为8路 固定
		unsigned long MachineCycle;	//机器周期，毫秒
		unsigned long FillTime;	//填充时间，毫秒
		unsigned long CycleTime;	//成型周期
		QList<int> keepList;	//100个压力点
		QString startCycle;	//生产数据开始时间
		MachineCycle = 100;
		FillTime = 100;
		CycleTime = 100;
		endCycle = Tool::GetCurrentDateTimeStr();
		startCycle = Tool::GetCurrentDateTimeStr();
		qDebug() << "MYTest ...";
		OrderMainOperation::GetInstance().OnAddModeCount(endCycle, temperList,
				MachineCycle, FillTime, CycleTime, keepList, startCycle);
	}
};

void qdebugLog(QtMsgType type, const char* msg)
{
    QFile out(AppInfo::GetInstance().getPath_Logs()+"qdebugLog.txt");
    out.open(QIODevice::WriteOnly | QIODevice::Append);
    QDataStream stream(&out);
    stream.writeRawData(msg,strlen(msg));
    stream.writeRawData("\r\n",2);
    out.close();
}

int main(int argc, char *argv[])
{    
	//设定字符集
	QTextCodec::setCodecForTr(QTextCodec::codecForName("GB2312"));
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("GB2312"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("GB2312"));
    {
        QString hex="5A 00 08 17 00 D8 0F 08 11 12 00 06 0F 08 11 15 00 06 08 00 C2 ED C9 CF B8 FC D0 C2 DB 6C";

        MESPack pack;
        pack.parse(MESTool::CSCPPackToQByteArray(hex));

        qDebug()<<MESTool::CSCPPackToQString(pack.getBody())<<endl;

        MESNoticeRequest request;
        if (request.decode(pack))
            qDebug()<<request.Text<<endl;
    }
	//共享内存
	MYTest *mytest;
	{
		Application a(argc, argv);
		a.init();      
        qInstallMsgHandler(qdebugLog);
		//输入法
		InputPannelContext *input = new InputPannelContext;
		a.setInputContext(input);

		//界面显示
        MainWindow w;
		w.setWindowFlags(Qt::FramelessWindowHint);
		w.setGeometry(QRect(0, 0, 800, 480));
		w.show();

		//模拟模次
		if(! AppInfo::sys_func_cfg.isIOCollect())
		{
		  mytest = new MYTest();
		  mytest->start();         
		}

		a.exec();
	}



	return 0;
}
