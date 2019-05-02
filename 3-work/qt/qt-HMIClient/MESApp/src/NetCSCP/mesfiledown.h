#ifndef MESFILEDOWN_H
#define MESFILEDOWN_H

#include <QObject>
#include "mesrequest.h"

class MESFileDown : public QObject
{
    QString name;  //下载文件名
    QByteArray ver; //版本
    QFile *file; //文件信息
    quint32 OffsetAddr; //开始下载地址
    quint32 FileSize; //文件大小
    quint32 downLen; //当前下载长度
    bool FileDownFlag; //文件下载标志

    Q_OBJECT
public:
    explicit MESFileDown(const QString &name,const QByteArray &ver,QObject *parent = 0);
    ~MESFileDown();

    bool start();

signals:
    

public slots:
    void sig_MESFileDownResponse(MESFileDownResponse response);
    void sig_MESFileDownStreamResponse(MESFileDownStreamResponse &response);
};

#endif // MESFILEDOWN_H
