#ifndef QLIVERTSPSERVER_H
#define QLIVERTSPSERVER_H

#include "RTSPServerSupportingHTTPStreaming.hh"
#include "RTSPServer.hh"
#include <QtCore>
#include "qmediafileinfo.h"

typedef void (*QLiveRTSPBCallClientConnect)(int socket, const char *ipAddress, unsigned short port);
typedef void (*QLiveRTSPBCallClinetClose)(int socket, const char *ipAddress, unsigned short port);
typedef void (*QLiveRTSPBCallClientDESCRIBE)(int socket, const char *ipAddress, unsigned short port,const char *urlSuffix);


class QLiveRTSPServer: public RTSPServer
{
public:
    static QLiveRTSPServer* createNew(UsageEnvironment& env, Port ourPort,
                                      UserAuthenticationDatabase* authDatabase,
                                      unsigned reclamationTestSeconds = 65);

    int getVideoBitrate(const QString &filePath);

    bool fileIsExists(const QString &name);
    QString fileDir() const;
    void setFileDir(const QString &fileDir);

    QString getFilePath(const QString &name);

    volatile int rtspClientSize;

    QLiveRTSPBCallClientConnect bkFunClinetConnect;
    QLiveRTSPBCallClinetClose   bkFunClientClose;
    QLiveRTSPBCallClientDESCRIBE bkFunClientDescribe;

    void setSocketSendBufSize(int size){
        this->fSocketSendBufSize=size;
    }

protected:
    QLiveRTSPServer(UsageEnvironment& env, int ourSocket, Port ourPort,
                    UserAuthenticationDatabase* authDatabase,
                    unsigned reclamationTestSeconds);

    // called only by createNew();
    virtual ~QLiveRTSPServer();

protected: // redefined virtual functions


    virtual ServerMediaSession* lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession);

    //有新连接
    virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr);

    virtual RTSPClientSession* createNewClientSession(u_int32_t sessionId);

private:
    QString mFileDir;
    int fSocketSendBufSize;
    friend class RTSPClientConnectionSupporting;
    friend class RTSPClientSessionEx;

public: // should be protected, but some old compilers complain otherwise

    class RTSPClientConnectionSupporting: public RTSPServer::RTSPClientConnection {
    protected:
        friend class QLiveRTSPServer;
        friend class RTSPClientSessionEx;

        RTSPClientConnectionSupporting(QLiveRTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr);
        virtual ~RTSPClientConnectionSupporting();

        virtual void handleCmd_DESCRIBE(char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr);

    protected:
        char mUrlSuffix[100];
        char mIpAddress[30];
        int mPort;
    };

    class RTSPClientSessionEx:public RTSPServer::RTSPClientSession {
    protected:
        friend class QLiveRTSPServer;
        friend class RTSPClientConnectionSupporting;

        RTSPClientSessionEx(RTSPServer& ourServer, u_int32_t sessionId);
        virtual ~RTSPClientSessionEx();
    };
};

#endif // QLIVERTSPSERVER_H
