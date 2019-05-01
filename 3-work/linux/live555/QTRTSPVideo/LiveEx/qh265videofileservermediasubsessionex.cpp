#include "qh265videofileservermediasubsessionex.h"
#include "qbytestreamfilesourceloop.h"
#include "H265VideoStreamFramer.hh"
#include <QDebug>

QH265VideoFileServerMediaSubsessionEx *QH265VideoFileServerMediaSubsessionEx::createNew(UsageEnvironment &env, const char *fileName, Boolean reuseFirstSource)
{
    return new QH265VideoFileServerMediaSubsessionEx(env, fileName, reuseFirstSource);
}

QH265VideoFileServerMediaSubsessionEx::QH265VideoFileServerMediaSubsessionEx(
        UsageEnvironment &env, const char *fileName, Boolean reuseFirstSource)
    :H265VideoFileServerMediaSubsession(env,fileName, reuseFirstSource)
{

}

QH265VideoFileServerMediaSubsessionEx::~QH265VideoFileServerMediaSubsessionEx()
{

}

FramedSource *QH265VideoFileServerMediaSubsessionEx::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    estBitrate = 500; // kbps, estimate

    // Create the video source:
    QByteStreamFileSourceLoop* fileSource = QByteStreamFileSourceLoop::createNew(envir(), fFileName);
    if (fileSource == NULL) return NULL;
    fFileSize = fileSource->fileSize();

    return H265VideoStreamFramer::createNew(envir(), fileSource);
    //return H265VideoFileServerMediaSubsession::createNewStreamSource(clientSessionId, estBitrate);
}
