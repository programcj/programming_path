#ifndef QH264VIDEOFILESERVERMEDIASUBSESSIONLOOP_H
#define QH264VIDEOFILESERVERMEDIASUBSESSIONLOOP_H

#include "H264VideoFileServerMediaSubsession.hh"

class QH264VideoFileServerMediaSubsessionLoop : public H264VideoFileServerMediaSubsession
{

public:
  static QH264VideoFileServerMediaSubsessionLoop*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

protected:
    QH264VideoFileServerMediaSubsessionLoop(UsageEnvironment& env,
              char const* fileName, Boolean reuseFirstSource);

    virtual ~QH264VideoFileServerMediaSubsessionLoop();

protected: // redefined virtual functions
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                              unsigned& estBitrate);

};

#endif // QH264VIDEOFILESERVERMEDIASUBSESSIONLOOP_H
