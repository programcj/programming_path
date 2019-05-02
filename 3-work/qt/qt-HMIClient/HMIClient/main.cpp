#include "ui/mainwindow.h"
#include <QApplication>
#include <QTextCodec>
#include <QDebug>
#include <QSemaphore>
#include <QFontDatabase>
#include <QDesktopWidget>
#include <qsystemdetection.h>

#include "ui/dialoginput.h"
#include "application.h"
#include "dao/sqlitebasehelper.h"
#include "dao/mesdispatchorder.h"
#include "netinterface/mespacket.h"
#include "tools/mestools.h"

static const char *msgHead[]={
    "DEBUG",
    "WARIN",
    "Critical",
    "Fatal",
    "INFO"
};

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    //yyyy-MM-dd hh:mm:ss ddd
    QString current_date_time = MESTools::GetCurrentDateTimeStr(); //QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    const char *file=context.file;

    if (file!=NULL && strrchr(file, '\\') != NULL)
        file = strrchr(file, '\\')+1;
    if (file!=NULL && strrchr(file, '/') != NULL)
        file = strrchr(file, '/')+1;
    //    if(gFileLog){
    //        QTextStream tWrite(gFileLog);
    //        QString msgText="%1 | %6 | %2:%3, %4 | %5\n";
    //        msgText = msgText.arg(msgHead[type]).arg(context.file).arg(context.line).arg(context.function).arg(localMsg.constData()).arg(current_date_time);
    //        //gFileLog->write(msgText.toLocal8Bit(), msgText.length());
    //        tWrite << msgText;
    //    }else{
    fprintf(stderr, "%s|%s|%s:%u,%s:%s\n", msgHead[type],
            current_date_time.toLocal8Bit().constData(), file,
            context.line, context.function, localMsg.constData());
    fflush(stderr);
    //    }
}

#if QT_VERSION < 0x050000
void qdebugLog(QtMsgType type, const char* msg)
{
    QFile out("/tmp/log.txt");
    out.open(QIODevice::WriteOnly | QIODevice::Append);
    QDataStream stream(&out);
    stream.writeRawData(msg,strlen(msg));
    stream.writeRawData("\r\n",2);
    out.close();
}
#endif

#include "hal/servicemodulekang.h"
#include "netinterface/netbroadhandler.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    //--/etc/systemd/system/multi-user.target.wants
#if QT_VERSION < 0x050400
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));        ////
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));      ////
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));  ////GB2312
#else
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

#if QT_VERSION < 0x050400
    qInstallMessageHandler(qdebugLog);
#else
    qInstallMessageHandler(myMessageOutput);
#endif
    qDebug()<< "start "<<MESTools::GetCurrentDateTimeStr() << QDir::currentPath();

#ifdef AM335X
    QDir::setCurrent("/home/root");
#endif

#ifdef Q_OS_LINUX_WEIQIAN
    QDir::setCurrent("/home/root");
      ::system("killall linuxdesktop00");
#endif

    //系统初始化
    Application a(argc, argv);
    a.init(); //接收消息队列启动，Mqtt连接任务启动

#ifdef AM335X
    // /dev/ttyO3 RFID
    int nIndex = QFontDatabase::addApplicationFont("/home/root/wqy-zenhei.ttc");
    if (nIndex != -1)
    {
        QStringList strList(QFontDatabase::applicationFontFamilies(nIndex));
        for(int i=0,j=strList.count();i<j;i++){
            qDebug() <<"Font family:"<< strList[i];
        }
        QFont font;
        font.setPointSize(10);
        font.setFamily("WenQuanYi Zen Hei");
        a.setFont(font);
        ::system("killall QtDemo");
    }
#endif
    qDebug()<<"========== show ===============";
    MainWindow *w=NULL;
    {
        QDesktopWidget *desk=QApplication::desktop();
        int wd=desk->width();
        int ht=desk->height();
        qDebug() << QString("Width:%1,Height:%2").arg(wd).arg(ht);
    }
    if(w==NULL){
        w=new MainWindow;
        w->setGeometry(QRect(0, 0, 1280, 800));
        w->show();
    }
    return a.exec();
}
