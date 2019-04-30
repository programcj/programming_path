#include "qlivemp4fileservermediasubsession.h"
#include "ByteStreamFileSource.hh"
#include "H264VideoStreamFramer.hh"
#include "qlivemp4bytestreamfilesource.h"
#include <QDebug>


QLiveMP4FileServerMediaSubsession *QLiveMP4FileServerMediaSubsession::createNew(UsageEnvironment &env, const char *fileName, Boolean reuseFirstSource)
{
    return new QLiveMP4FileServerMediaSubsession(env, fileName,
                                                 reuseFirstSource);
}

QLiveMP4FileServerMediaSubsession::QLiveMP4FileServerMediaSubsession(
        UsageEnvironment &env,
        const char *fileName,
        Boolean reuseFirstSource):
    H264VideoFileServerMediaSubsession(env, fileName, reuseFirstSource)
{
    fBitrate=500;
    qDebug()<<"QLiveMP4FileServerMediaSubsession";
}

QLiveMP4FileServerMediaSubsession::~QLiveMP4FileServerMediaSubsession()
{
    qDebug()<<"~QLiveMP4FileServerMediaSubsession";
}

FramedSource *QLiveMP4FileServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
    estBitrate = getBitrate(); // kbps, estimate

    printf("QLiveMP4ByteStreamFileSource crate:%s,estBitrate:%d\n",fFileName, estBitrate);
    QLiveMP4ByteStreamFileSource* fileSource =
            QLiveMP4ByteStreamFileSource::createNew(envir(), fFileName);
    if (fileSource == NULL)
        return NULL;
    //fFileSize = fileSource->fileSize();
    // Create a framer for the Video Elementary Stream:
    return H264VideoStreamFramer::createNew(envir(), fileSource);
}

int QLiveMP4FileServerMediaSubsession::getBitrate() const
{
    return fBitrate;
}

void QLiveMP4FileServerMediaSubsession::setBitrate(int value)
{
    fBitrate = value;
}
