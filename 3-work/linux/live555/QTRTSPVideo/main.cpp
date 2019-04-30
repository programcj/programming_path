#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "mainwindow.h"
#include <QApplication>
#include <QByteArray>
#include <jzlxsapplication.h>

#include <QDebug>
#include <QtCore>
#include <qrtspserver.h>
#include <QTextCodec>

static void QDebugMessageToThis(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString text;
    switch((int)type)
    {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
    }

    QString context_info = QString("(%1:%2)").arg(QString(context.file)).arg(context.line);
    QString current_date_time = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
    QString current_date = QString("(%1)").arg(current_date_time);
    QString message = QString("%1 %2 %3 %4").arg(current_date).arg(text).arg(context_info).arg(msg);

    QTextStream textStream(stdout, QIODevice::WriteOnly);
    textStream.setCodec("gb2312");  //指定编码
    textStream<<message<<"\n";

    QFile file("log.txt");
    if(file.size()>1024*1024*5) {
        file.open(QIODevice::ReadWrite | QIODevice::Truncate);
    } else
        file.open(QIODevice::ReadWrite | QIODevice::Append);
    QTextStream stream(&file);
    stream << message << "\r\n";
    file.flush();
    file.close();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(QDebugMessageToThis);

    ///QTextCodec *codec=QTextCodec::codecForName("GB2312");
    ///QTextCodec::setCodecForLocale(codec);//

    qDebug()<<"FFmpeg初始化";
    QMediaFileInfo::Init();
    JZLXSApplication a(argc, argv);
    MainWindow w;

    //QMediaFileInfo info("G:\\test.264");

    w.show();

    //    QRTSPServer::getInstance()->setVideosDir("G:\\unit_2\\videos");
    //    QRTSPServer::getInstance()->startRtspServer();

    return a.exec();
}
