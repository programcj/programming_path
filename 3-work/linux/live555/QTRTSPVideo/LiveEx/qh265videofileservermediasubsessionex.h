#ifndef H265VIDEOFILESERVERMEDIASUBSESSIONEX_H
#define H265VIDEOFILESERVERMEDIASUBSESSIONEX_H

#include "H265VideoFileServerMediaSubsession.hh"

class QH265VideoFileServerMediaSubsessionEx : public H265VideoFileServerMediaSubsession
{
public:
  static QH265VideoFileServerMediaSubsessionEx*
  createNew(UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource);

protected:
  QH265VideoFileServerMediaSubsessionEx(UsageEnvironment& env,
                      char const* fileName, Boolean reuseFirstSource);

  virtual ~QH265VideoFileServerMediaSubsessionEx();

protected: // redefined virtual functions

  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                          unsigned& estBitrate);
};

#endif // H265VIDEOFILESERVERMEDIASUBSESSIONEX_H
