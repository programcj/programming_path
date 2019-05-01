#ifndef QLIVEMP4FILESERVERMEDIASUBSESSION_H
#define QLIVEMP4FILESERVERMEDIASUBSESSION_H

#include "H264VideoFileServerMediaSubsession.hh"


class QLiveMP4FileServerMediaSubsession : public H264VideoFileServerMediaSubsession
{
public:
    static QLiveMP4FileServerMediaSubsession*
        createNew(UsageEnvironment& env, char const* fileName,
                Boolean reuseFirstSource);

    int getBitrate() const;
    void setBitrate(int value);

protected:
    QLiveMP4FileServerMediaSubsession(UsageEnvironment& env,
                                      char const* fileName, Boolean reuseFirstSource);
    virtual ~QLiveMP4FileServerMediaSubsession();

    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                unsigned& estBitrate);

private :
    int fBitrate;
};

#endif // QLIVEMP4FILESERVERMEDIASUBSESSION_H
