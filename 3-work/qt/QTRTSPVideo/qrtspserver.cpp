#include "qrtspserver.h"
#include <BasicUsageEnvironment.hh>
#include "qlivertspserver.h"
#include "LiveEx/qbasicusageenvironment.h"

QRTSPServer *QRTSPServer::instance=NULL;

static void RTSPBCallClientConnect(int socket, const char *ipAddress, unsigned short port)
{
    QString ip=QString("%1:%2").arg(ipAddress).arg(port);
    emit QRTSPServer::getInstance()->rtspClientConnect(ip);
}

static void RTSPBCallClinetClose(int socket, const char *ipAddress, unsigned short port)
{
    QString ip=QString("%1:%2").arg(ipAddress).arg(port);
    emit QRTSPServer::getInstance()->rtspClientClose(ip);
}

static void RTSPBCallClientDESCRIBE(int socket, const char *ipAddress, unsigned short port,const char *urlSuffix){
    QString ip=QString("%1:%2").arg(ipAddress).arg(port);

    QString urlDecode = QUrl::fromPercentEncoding(urlSuffix);
    QString urlSuf(urlDecode);

    emit QRTSPServer::getInstance()->rtspClientDescribe(ip,urlSuf);
}

QString QRTSPServer::getVideosDir() const
{
    return mVideosDir;
}

void QRTSPServer::setVideosDir(const QString &videosDir)
{
    mVideosDir = videosDir;
}

QRTSPServer *QRTSPServer::getInstance()
{
    return instance;
}

QRTSPServer::QRTSPServer(QObject *parent) : QThread(parent)
{
    instance=this;
    _rtspLoopFlag=0;
}

QRTSPServer::~QRTSPServer()
{

}

/**
 * @brief QRTSPServer::run
 * 启动rtsp服务器
 */
void QRTSPServer::run()
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = QBasicUsageEnvironment::createNew(*scheduler);
    UserAuthenticationDatabase* authDB = NULL;
    QLiveRTSPServer* rtspServer;
    portNumBits rtspServerPortNum = 8880;

    rtspServer = QLiveRTSPServer::createNew(*env, rtspServerPortNum, authDB);
    if (rtspServer == NULL) {
        rtspServerPortNum = 8554;
        rtspServer = QLiveRTSPServer::createNew(*env, rtspServerPortNum, authDB);
    }

    if (rtspServer == NULL) {
        qDebug() << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }
    qDebug() << "LIVE555 Media Server\n";

    //rtspServer->disableStreamingRTPOverTCP(); //禁止使用TCP方式
    rtspServer->setSocketSendBufSize(1024*1024*2);

    QString urlPrefix(rtspServer->rtspURLPrefix());
    qDebug()<<"start:" << urlPrefix;

    emit this->rtspServerStart(urlPrefix);

    _rtspLoopFlag=0;
    rtspServer->setFileDir(this->mVideosDir);

    rtspServer->bkFunClinetConnect=RTSPBCallClientConnect;
    rtspServer->bkFunClientClose=RTSPBCallClinetClose;
    rtspServer->bkFunClientDescribe=RTSPBCallClientDESCRIBE;

    env->taskScheduler().doEventLoop(&_rtspLoopFlag); // does not return
    RTSPServer::close(rtspServer);
    if(env->reclaim()){
        qDebug()<<"delete UsageEnvironment";
        rtspServer= NULL;
    }
    delete scheduler;

    while(!isInterruptionRequested()){

    }
    emit this->rtspServerClose();
    qDebug() << "exit thread";
}

void QRTSPServer::startRtspServer()
{
    //wait stop
    start();
}

void QRTSPServer::stopRtspServer()
{
    _rtspLoopFlag=1;
    requestInterruption();
    quit();
    wait();
}

void QRTSPServer::start(QThread::Priority p)
{
    qDebug()<<"start";
    QThread::start(p);
}
