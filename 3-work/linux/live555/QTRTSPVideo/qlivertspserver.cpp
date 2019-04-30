#include "qlivertspserver.h"
#include <QFile>
#include <liveMedia.hh>
#include <string.h>
#include "LiveEx/qlivemp4fileservermediasubsession.h"
#include "LiveEx/qh264videofileservermediasubsessionloop.h"
#include "LiveEx/qh265videofileservermediasubsessionex.h"
#include <QTextCodec>
#include <QDebug>
#include "GroupsockHelper.hh"

QLiveRTSPServer *QLiveRTSPServer::createNew(UsageEnvironment &env, Port ourPort, UserAuthenticationDatabase *authDatabase, unsigned reclamationTestSeconds)
{
    int ourSocket = setUpOurSocket(env, ourPort);
    if (ourSocket == -1) return NULL;

    return new QLiveRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds);
}

#if 0
RTSPServerSupportingHTTPStreaming
#endif
QLiveRTSPServer::QLiveRTSPServer(UsageEnvironment& env, int ourSocket,
                                 Port ourPort,
                                 UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
    : RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
{
    rtspClientSize=0;
    this->bkFunClientClose=NULL;
    this->bkFunClinetConnect=NULL;
    this->bkFunClientDescribe=NULL;
}

QLiveRTSPServer::~QLiveRTSPServer()
{

}

int QLiveRTSPServer::getVideoBitrate(const QString &filePath)
{
    QMediaFileInfo info(filePath);
    return info.getVideoBitRae();
}

QString QLiveRTSPServer::fileDir() const
{
    return mFileDir;
}

void QLiveRTSPServer::setFileDir(const QString &fileDir)
{
    mFileDir = fileDir;
}

QString QLiveRTSPServer::getFilePath(const QString &name)
{
    QFileInfo fileInfo;
    QDir dir(fileDir());
    fileInfo.setFile(dir,name);
    return fileInfo.filePath();
}

bool QLiveRTSPServer::fileIsExists(const QString &name)
{
    QFileInfo fileInfo;
    QDir dir(fileDir());
    fileInfo.setFile(dir,name);
    return fileInfo.exists();
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
                                        char const* fileName, int bitrate); // forward

ServerMediaSession *QLiveRTSPServer::lookupServerMediaSession(const char *streamName, Boolean isFirstLookupInSession)
{
    QString urlName=QString::fromLocal8Bit(streamName);
    QString urlDecode = urlName ;
    if(strncmp(streamName,"%",1)==0){
        urlDecode=QUrl::fromPercentEncoding(urlName.toLocal8Bit().data());//URL code 转换
        //QString urlEncode = QUrl::toPercentEncoding(urlDecode);
    }
    QString name(urlDecode);
    QString filePath= getFilePath(name);
    QByteArray filePathArray=filePath.toLocal8Bit();
    const char *filePathStr=filePathArray.constData();

    //printf("--------QLiveRTSPServer::lookupServerMediaSession(%s)-------------------\n", streamName);
//    FILE *fp=fopen(filePathStr,"rb");
//    if(fp){
//        printf("File fopen ok %s\n", filePathStr);
//        fclose(fp);
//    } else { ///intrace
//        qDebug()<<"urlDecode:"<<urlDecode;
//        printf("%s\n", filePathStr);
//        printf("---------------------------\n");
//        printf("File not fopen %s\n", filePathStr);
//    }
//    qDebug()<<"RTSP File "<< filePath << filePathStr;

    Boolean fileExists = fileIsExists(name)==true?True:False;

    ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
    Boolean smsExists = sms != NULL;

    qDebug()<<"lookupServerMediaSession " << name;

    if (!fileExists) {
        if (smsExists) {
            // "sms" was created for a file that no longer exists. Remove it:
            removeServerMediaSession(sms);
            sms = NULL;
        }

        return NULL;
    } else {
        if (smsExists && isFirstLookupInSession) {
            // Remove the existing "ServerMediaSession" and create a new one, in case the underlying
            // file has changed in some way:
            removeServerMediaSession(sms);
            sms = NULL;
        }

        if (sms == NULL) {
            sms = createNewSMS(envir(), filePathStr, getVideoBitrate(filePath));
            addServerMediaSession(sms);
        }
        //        fclose(fid);
        return sms;
    }
}

// Special code for handling Matroska files:
struct MatroskaDemuxCreationState {
    MatroskaFileServerDemux* demux;
    char watchVariable;
};

static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* clientData) {
    MatroskaDemuxCreationState* creationState = (MatroskaDemuxCreationState*)clientData;
    creationState->demux = newDemux;
    creationState->watchVariable = 1;
}
// END Special code for handling Matroska files:

// Special code for handling Ogg files:
struct OggDemuxCreationState {
    OggFileServerDemux* demux;
    char watchVariable;
};

static void onOggDemuxCreation(OggFileServerDemux* newDemux, void* clientData) {
    OggDemuxCreationState* creationState = (OggDemuxCreationState*)clientData;
    creationState->demux = newDemux;
    creationState->watchVariable = 1;
}
// END Special code for handling Ogg files:

#define NEW_SMS(description) do {\
    char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
    sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
    } while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
                                        char const* fileName, int bitrate) {
    char const* extension = strrchr(fileName, '.');
    if (extension == NULL)
        return NULL;

    ServerMediaSession* sms = NULL;
    Boolean const reuseSource = False;

    printf("create New Sms:%s\n", fileName);

    if(strcmp(extension, ".264")==0
            || stricmp(extension, ".h264")==0) {
        NEW_SMS("H264 Video");
        OutPacketBuffer::maxSize = 2000000;
        //2000000
        sms->addSubsession(QH264VideoFileServerMediaSubsessionLoop::createNew(env,fileName,reuseSource));
    } else  if(strcmp(extension, ".mp4")==0) {
        NEW_SMS("MP4 Video");
        OutPacketBuffer::maxSize = 2000000;
        QLiveMP4FileServerMediaSubsession *subs=QLiveMP4FileServerMediaSubsession::createNew(env,fileName,reuseSource);
        subs->setBitrate(bitrate);
        sms->addSubsession(subs);
    } else if (strcmp(extension, ".aac") == 0) {
        // Assumed to be an AAC Audio (ADTS format) file:
        NEW_SMS("AAC Audio");
        sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".amr") == 0) {
        // Assumed to be an AMR Audio file:
        NEW_SMS("AMR Audio");
        sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".ac3") == 0) {
        // Assumed to be an AC-3 Audio file:
        NEW_SMS("AC-3 Audio");
        sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".m4e") == 0) {
        // Assumed to be a MPEG-4 Video Elementary Stream file:
        NEW_SMS("MPEG-4 Video");
        sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".264") == 0) {
        // Assumed to be a H.264 Video Elementary Stream file:
        NEW_SMS("H.264 Video");
        OutPacketBuffer::maxSize = 2000000; // allow for some possibly large H.264 frames
        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".265") == 0
               || strcmp(extension, ".H265") == 0
               || strcmp(extension, ".h265") == 0) {
        // Assumed to be a H.265 Video Elementary Stream file:
        NEW_SMS("H.265 Video");
        OutPacketBuffer::maxSize = 2000000; // allow for some possibly large H.265 frames
        qDebug()<<"h265 video";
        sms->addSubsession(QH265VideoFileServerMediaSubsessionEx::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mp3") == 0) {
        // Assumed to be a MPEG-1 or 2 Audio file:
        NEW_SMS("MPEG-1 or 2 Audio");
        // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
        //#define STREAM_USING_ADUS 1
        // To also reorder ADUs before streaming, uncomment the following:
        //#define INTERLEAVE_ADUS 1
        // (For more information about ADUs and interleaving,
        //  see <http://www.live555.com/rtp-mp3/>)
        Boolean useADUs = False;
        Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
        useADUs = True;
#ifdef INTERLEAVE_ADUS
        unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
        unsigned const interleaveCycleSize
                = (sizeof interleaveCycle)/(sizeof (unsigned char));
        interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
        sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
    } else if (strcmp(extension, ".mpg") == 0) {
        // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
        NEW_SMS("MPEG-1 or 2 Program Stream");
        MPEG1or2FileServerDemux* demux
                = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAudioServerMediaSubsession());
    } else if (strcmp(extension, ".vob") == 0) {
        // Assumed to be a VOB (MPEG-2 Program Stream, with AC-3 audio) file:
        NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
        MPEG1or2FileServerDemux* demux
                = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
    } else if (strcmp(extension, ".ts") == 0) {
        // Assumed to be a MPEG Transport Stream file:
        // Use an index file name that's the same as the TS file name, except with ".tsx":
        unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
        char* indexFileName = new char[indexFileNameLen];
        sprintf(indexFileName, "%sx", fileName);
        NEW_SMS("MPEG Transport Stream");
        sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
        delete[] indexFileName;
    } else if (strcmp(extension, ".wav") == 0) {
        // Assumed to be a WAV Audio file:
        NEW_SMS("WAV Audio Stream");
        // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
        // change the following to True:
        Boolean convertToULaw = False;
        sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
    } else if (strcmp(extension, ".dv") == 0) {
        // Assumed to be a DV Video file
        // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
        OutPacketBuffer::maxSize = 2000000;

        NEW_SMS("DV Video");
        sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
        // Assumed to be a Matroska file (note that WebM ('.webm') files are also Matroska files)
        OutPacketBuffer::maxSize = 2000000; // allow for some possibly large VP8 or VP9 frames
        NEW_SMS("Matroska video+audio+(optional)subtitles");

        // Create a Matroska file server demultiplexor for the specified file.
        // (We enter the event loop to wait for this to complete.)
        MatroskaDemuxCreationState creationState;
        creationState.watchVariable = 0;
        MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession* smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
            sms->addSubsession(smss);
        }
    } else if (strcmp(extension, ".ogg") == 0 || strcmp(extension, ".ogv") == 0 || strcmp(extension, ".opus") == 0) {
        // Assumed to be an Ogg file
        NEW_SMS("Ogg video and/or audio");

        // Create a Ogg file server demultiplexor for the specified file.
        // (We enter the event loop to wait for this to complete.)
        OggDemuxCreationState creationState;
        creationState.watchVariable = 0;
        OggFileServerDemux::createNew(env, fileName, onOggDemuxCreation, &creationState);
        env.taskScheduler().doEventLoop(&creationState.watchVariable);

        ServerMediaSubsession* smss;
        while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
            sms->addSubsession(smss);
        }
    }

    return sms;
}

GenericMediaServer::ClientConnection *QLiveRTSPServer::createNewClientConnection(int clientSocket, sockaddr_in clientAddr)
{
	ignoreSigPipeOnSocket(clientSocket);
	increaseSendBufferTo(envir(), clientSocket, fSocketSendBufSize);
    return new RTSPClientConnectionSupporting(*this, clientSocket, clientAddr); //有新连接
}

RTSPServer::RTSPClientSession *QLiveRTSPServer::createNewClientSession(u_int32_t sessionId)
{
    return new RTSPClientSessionEx(*this, sessionId); //新连接有新的session播放
}


QLiveRTSPServer::RTSPClientConnectionSupporting::RTSPClientConnectionSupporting(
        QLiveRTSPServer &ourServer, int clientSocket, sockaddr_in clientAddr):
    RTSPClientConnection(ourServer, clientSocket, clientAddr) {
    //    int inet_pton(int family, const char *strptr, void *addrptr);     //   返回值：若成功则为1，若输入不是有效的表达式则为0，若出错则为-1
    //    const char * inet_ntop(int family, const void *addrptr, char *strptr, size_t len);     //将数值格式转化为点分十进制的ip地址格式
    //char *ptr = inet_ntop(AF_INET,&clientAddr.sin_addr, ipAddress, sizeof(ipAddress)); // 代替 ptr = inet_ntoa(foo.sin_addr)

    AddressString addr(clientAddr);

    mPort=ntohs(clientAddr.sin_port);
    strncpy(mIpAddress,addr.val(), sizeof mIpAddress);

    memset(mUrlSuffix,0,sizeof mUrlSuffix);

    qDebug()<<"新连接 "<<addr.val()<<":"<<mPort;

    ourServer.rtspClientSize++;
    if(ourServer.bkFunClinetConnect)
        ourServer.bkFunClinetConnect(clientSocket,mIpAddress,mPort);
}

QLiveRTSPServer::RTSPClientConnectionSupporting::~RTSPClientConnectionSupporting()
{
    QLiveRTSPServer *ourServer=(QLiveRTSPServer*)&fOurRTSPServer;
    if(ourServer->bkFunClientClose)
        ourServer->bkFunClientClose(fOurSocket,mIpAddress,mPort);
    ourServer->rtspClientSize--;
    qDebug()<<"断开连接";
}

void QLiveRTSPServer::RTSPClientConnectionSupporting::handleCmd_DESCRIBE(const char *urlPreSuffix, const char *urlSuffix, const char *fullRequestStr)
{
    // DESCRIBE rtsp://10.3.8.202:554 RTSP/1.0
#if 0
    const char *ptr=fullRequestStr;
    const char *end=ptr;
    ptr=strstr(fullRequestStr,"rtsp:");
    if(!ptr)
        ptr=strstr(fullRequestStr,"RTSP:");

    if(ptr){
        end=strstr(ptr,"rtsp/");
        if(!end){
            end=strstr(ptr,"RTSP/");
        }
        if(end){
            strncpy(mUrlFullAddress,ptr, end-ptr-1);
        }
    }
#endif
    strncpy(this->mUrlSuffix,urlSuffix,sizeof this->mUrlSuffix);

    qDebug()<< mIpAddress << " DESCRIBE:" <<urlSuffix <<"|"<<this->mUrlSuffix;
    QLiveRTSPServer *ourServer=(QLiveRTSPServer*)&fOurRTSPServer;
    if(ourServer->bkFunClientDescribe)
        ourServer->bkFunClientDescribe(fOurSocket,mIpAddress,mPort,this->mUrlSuffix);

    RTSPClientConnection::handleCmd_DESCRIBE(urlPreSuffix,urlSuffix,fullRequestStr);
}

QLiveRTSPServer::RTSPClientSessionEx::RTSPClientSessionEx(
        RTSPServer& ourServer, u_int32_t sessionId)
    :RTSPServer::RTSPClientSession(ourServer, sessionId)
{

}

QLiveRTSPServer::RTSPClientSessionEx::~RTSPClientSessionEx()
{

}
