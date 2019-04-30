#include "qbytestreamfilesourceloop.h"
#include "InputFile.hh"
#include "GroupsockHelper.hh"

#define READ_FROM_FILES_SYNCHRONOUSLY

////////// QByteStreamFileSourceLoop //////////
QByteStreamFileSourceLoop *QByteStreamFileSourceLoop::createNew(
        UsageEnvironment& env, char const* fileName,
        unsigned preferredFrameSize, unsigned playTimePerFrame) {
    FILE* fid = OpenInputFile(env, fileName);
    if (fid == NULL)
        return NULL;

    QByteStreamFileSourceLoop* newSource =
            new QByteStreamFileSourceLoop(env, fid, preferredFrameSize,
                    playTimePerFrame);
    newSource->fFileSize = GetFileSize(fileName, fid);
    return newSource;
}

QByteStreamFileSourceLoop*
QByteStreamFileSourceLoop::createNew(UsageEnvironment& env, FILE* fid,
        unsigned preferredFrameSize, unsigned playTimePerFrame) {
    if (fid == NULL)
        return NULL;

    QByteStreamFileSourceLoop* newSource =
            new QByteStreamFileSourceLoop(env, fid, preferredFrameSize,
                    playTimePerFrame);
    newSource->fFileSize = GetFileSize(NULL, fid);

    return newSource;
}

void QByteStreamFileSourceLoop::seekToByteAbsolute(u_int64_t byteNumber,
        u_int64_t numBytesToStream) {
    SeekFile64(fFid, (int64_t) byteNumber, SEEK_SET);

    fNumBytesToStream = numBytesToStream;
    fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void QByteStreamFileSourceLoop::seekToByteRelative(int64_t offset,
        u_int64_t numBytesToStream) {
    SeekFile64(fFid, offset, SEEK_CUR);

    fNumBytesToStream = numBytesToStream;
    fLimitNumBytesToStream = fNumBytesToStream > 0;
}

void QByteStreamFileSourceLoop::seekToEnd() {
    SeekFile64(fFid, 0, SEEK_END);
}

QByteStreamFileSourceLoop::QByteStreamFileSourceLoop(
        UsageEnvironment& env, FILE* fid, unsigned preferredFrameSize,
        unsigned playTimePerFrame) :
        FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(
                preferredFrameSize), fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(
                0), fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(
                0) {
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    makeSocketNonBlocking (fileno(fFid));
#endif

    // Test whether the file is seekable
    fFidIsSeekable = FileIsSeekable(fFid);
}

QByteStreamFileSourceLoop::~QByteStreamFileSourceLoop() {
    if (fFid == NULL)
        return;

#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
#endif

    CloseInputFile(fFid);
}

void QByteStreamFileSourceLoop::doGetNextFrame() {
#define CJ_WHILLE_PLAY 1
#if CJ_WHILLE_PLAY  //循环读取
    if (feof(fFid) || ferror(fFid)
            || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
        fseek(fFid, 0, SEEK_SET);
    }
#else
    if (feof(fFid) || ferror(fFid)
            || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
        handleClosure();
        return;
    }
#endif

#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    doReadFromFile();
#else
    if (!fHaveStartedReading) {
        // Await readable data from the file: 等待来自文件的可读数据
        envir().taskScheduler().turnOnBackgroundReadHandling(fileno(fFid),
                (TaskScheduler::BackgroundHandlerProc*) &fileReadableHandler,
                this);
        fHaveStartedReading = True;
    }
#endif
}

void QByteStreamFileSourceLoop::doStopGettingFrames() {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    envir().taskScheduler().turnOffBackgroundReadHandling(fileno(fFid));
    fHaveStartedReading = False;
#endif
}

void QByteStreamFileSourceLoop::fileReadableHandler(
        QByteStreamFileSourceLoop* source, int /*mask*/) {
    if (!source->isCurrentlyAwaitingData()) {
        source->doStopGettingFrames(); // we're not ready for the data yet
        return;
    }
    source->doReadFromFile();
}

void QByteStreamFileSourceLoop::doReadFromFile() {
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t) fMaxSize) {
        fMaxSize = (unsigned) fNumBytesToStream;
    }
    if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
        fMaxSize = fPreferredFrameSize;
    }
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    fFrameSize = fread(fTo, 1, fMaxSize, fFid);
#else
    if (fFidIsSeekable) {
        fFrameSize = fread(fTo, 1, fMaxSize, fFid);
    } else {
        // For non-seekable files (e.g., pipes), call "read()" rather than "fread()", to ensure that the read doesn't block:
        fFrameSize = read(fileno(fFid), fTo, fMaxSize);
    }
#endif
    if (fFrameSize == 0) {
        handleClosure();
        return;
    }
    fNumBytesToStream -= fFrameSize;

    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
        if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
            // This is the first frame, so use the current time:
            gettimeofday(&fPresentationTime, NULL);
        } else {
            // Increment by the play time of the previous data:
            unsigned uSeconds = fPresentationTime.tv_usec + fLastPlayTime;
            fPresentationTime.tv_sec += uSeconds / 1000000;
            fPresentationTime.tv_usec = uSeconds % 1000000;
        }

        // Remember the play time of this data:
        fLastPlayTime = (fPlayTimePerFrame * fFrameSize) / fPreferredFrameSize;
        fDurationInMicroseconds = fLastPlayTime;
    } else {
        // We don't know a specific play time duration for this data,
        // so just record the current time as being the 'presentation time':
        gettimeofday(&fPresentationTime, NULL);
    }

//    log_d("fMaxSize:%d, fFrameSize:%d, fDurationInMicroseconds:%d", fMaxSize,
//            fFrameSize, fDurationInMicroseconds);
    // Inform the reader that he has data:
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    // To avoid possible infinite recursion, we need to return to the event loop to do this:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*) FramedSource::afterGetting, this);
#else
    // Because the file read was done from the event loop, we can call the
    // 'after getting' function directly, without risk of infinite recursion:
    FramedSource::afterGetting(this);
#endif
}
