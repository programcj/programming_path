#ifndef QBYTESTREAMFILESOURCELOOP_H
#define QBYTESTREAMFILESOURCELOOP_H

#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif
/**
 * @brief The QByteStreamFileSourceLoop class
 * ByteStreamFileSource ex
 */
class QByteStreamFileSourceLoop : public FramedFileSource
{
public:
  static QByteStreamFileSourceLoop* createNew(UsageEnvironment& env,
                     char const* fileName,
                     unsigned preferredFrameSize = 0,
                     unsigned playTimePerFrame = 0);

  static QByteStreamFileSourceLoop* createNew(UsageEnvironment& env,
                     FILE* fid,
                     unsigned preferredFrameSize = 0,
                     unsigned playTimePerFrame = 0);
      // an alternative version of "createNew()" that's used if you already have
      // an open file.

  u_int64_t fileSize() const { return fFileSize; }
      // 0 means zero-length, unbounded, or unknown

  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
    // if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteRelative(int64_t offset, u_int64_t numBytesToStream = 0);
  void seekToEnd(); // to force EOF handling on the next read

protected:
  QByteStreamFileSourceLoop(UsageEnvironment& env,
               FILE* fid,
               unsigned preferredFrameSize,
               unsigned playTimePerFrame);
    // called only by createNew()

  virtual ~QByteStreamFileSourceLoop();

  static void fileReadableHandler(QByteStreamFileSourceLoop* source, int mask);
  void doReadFromFile();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

protected:
  u_int64_t fFileSize;

private:
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  Boolean fFidIsSeekable;
  unsigned fLastPlayTime;
  Boolean fHaveStartedReading;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

#endif // QBYTESTREAMFILESOURCELOOP_H
