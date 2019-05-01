#include "qh264videofileservermediasubsessionloop.h"
#include "H264VideoFileServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "H264VideoStreamFramer.hh"
#include "qbytestreamfilesourceloop.h"
#include <QDebug>

QH264VideoFileServerMediaSubsessionLoop *QH264VideoFileServerMediaSubsessionLoop::createNew(UsageEnvironment &env, const char *fileName, Boolean reuseFirstSource)
{
    return new QH264VideoFileServerMediaSubsessionLoop(env, fileName,
                                                       reuseFirstSource);
}

QH264VideoFileServerMediaSubsessionLoop::QH264VideoFileServerMediaSubsessionLoop(
        UsageEnvironment &env,
        const char *fileName,
        Boolean reuseFirstSource):H264VideoFileServerMediaSubsession(env, fileName, reuseFirstSource)
{
     qDebug()<<"QH264VideoFileServerMediaSubsessionLoop";
}

QH264VideoFileServerMediaSubsessionLoop::~QH264VideoFileServerMediaSubsessionLoop()
{
    qDebug()<<"~QH264VideoFileServerMediaSubsessionLoop";
}

FramedSource *QH264VideoFileServerMediaSubsessionLoop::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
    estBitrate = 500; // kbps, estimate
    // Create the video source:
    QByteStreamFileSourceLoop* fileSource = QByteStreamFileSourceLoop::createNew(envir(), fFileName);
    if (fileSource == NULL) return NULL;
    fFileSize = fileSource->fileSize();
    qDebug()<<"createNewStreamSource QByteStreamFileSourceLoop";

    // Create a framer for the Video Elementary Stream:
    return H264VideoStreamFramer::createNew(envir(), fileSource);
}
