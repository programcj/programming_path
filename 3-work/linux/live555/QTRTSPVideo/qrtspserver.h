#ifndef QRTSPSERVER_H
#define QRTSPSERVER_H

#include <QObject>
#include <QThread>
#include <QtCore>

class QRTSPServer : public QThread
{
    Q_OBJECT

    static QRTSPServer *instance;
    volatile char _rtspLoopFlag;
    QString  mVideosDir;
    int clinetConnectNumber; //客户端连接数量
public:

    unsigned short mPort; //

    static QRTSPServer *getInstance();

    explicit QRTSPServer(QObject *parent = 0);
    ~QRTSPServer();

    virtual void run() Q_DECL_OVERRIDE ;

    void startRtspServer();

    void stopRtspServer();

    QString getVideosDir() const;

    void setVideosDir(const QString &videosDir);

    int getClinetConnectNumber() const {
        return clinetConnectNumber;
    }

Q_SIGNALS:
    void rtspServerStart(const QString &urlPrefix);
    void rtspServerClose();

    void rtspClientConnect(const QString ip);
    void rtspClientClose(const QString ip);
    void rtspClientDescribe(const QString &clientIP,const QString &urlSuffix);

public slots:

protected:
    virtual  void start(Priority = InheritPriority);
};

#endif // QRTSPSERVER_H
