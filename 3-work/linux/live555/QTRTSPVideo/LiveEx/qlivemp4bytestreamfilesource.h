#ifndef QLIVEMP4BYTESTREAMFILESOURCE_H
#define QLIVEMP4BYTESTREAMFILESOURCE_H


#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

class QLiveMP4ByteStreamFileSource : public FramedSource
{
    void *bindData;

public:
    static QLiveMP4ByteStreamFileSource* createNew(UsageEnvironment& env,
                                                   char const* fileName, unsigned preferredFrameSize = 0,
                                                   unsigned playTimePerFrame = 0);

    static QLiveMP4ByteStreamFileSource* createNew(UsageEnvironment& env,
                                                   char const* fileName, FILE* fid, unsigned preferredFrameSize = 0,
                                                   unsigned playTimePerFrame = 0);
protected:

    QLiveMP4ByteStreamFileSource(UsageEnvironment& env, const char *fileName,
                                 unsigned preferredFrameSize, unsigned playTimePerFrame);
    // called only by createNew()
    virtual ~QLiveMP4ByteStreamFileSource();

   // int doReadVideoFrame(AVPacket &packet);
    virtual unsigned maxFrameSize() const;

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();

};

#endif // QLIVEMP4BYTESTREAMFILESOURCE_H
