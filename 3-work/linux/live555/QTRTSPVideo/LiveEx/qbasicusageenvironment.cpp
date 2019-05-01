#include "qbasicusageenvironment.h"

#include <stdio.h>

////////// BasicUsageEnvironment //////////

#if defined(__WIN32__) || defined(_WIN32)
extern "C" int initializeWinsockIfNecessary();
#endif

QBasicUsageEnvironment *QBasicUsageEnvironment::createNew(TaskScheduler &taskScheduler)
{
    return new QBasicUsageEnvironment(taskScheduler);
}

QBasicUsageEnvironment::QBasicUsageEnvironment(TaskScheduler& taskScheduler)
    : BasicUsageEnvironment0(taskScheduler) {
#if defined(__WIN32__) || defined(_WIN32)
    if (!initializeWinsockIfNecessary()) {
        setResultErrMsg("Failed to initialize 'winsock': ");
        reportBackgroundError();
        internalError();
    }
#endif
}

QBasicUsageEnvironment::~QBasicUsageEnvironment()
{
}

int QBasicUsageEnvironment::getErrno() const
{
#if defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
    return WSAGetLastError();
#else
    return errno;
#endif
}

UsageEnvironment& QBasicUsageEnvironment::operator<<(char const* str) {
    if (str == NULL)
        str = "(NULL)"; // sanity check
    //fprintf(stderr, "%s", str);
    qDebug()<<str;
    return *this;
}

UsageEnvironment& QBasicUsageEnvironment::operator<<(int i) {
    qDebug()<<i;
    return *this;
}

UsageEnvironment& QBasicUsageEnvironment::operator<<(unsigned u) {
    qDebug()<<u;
    return *this;
}

UsageEnvironment& QBasicUsageEnvironment::operator<<(double d) {
    qDebug()<<d;
    return *this;
}

UsageEnvironment& QBasicUsageEnvironment::operator<<(void* p) {
    QString string;
    string.sprintf("%p", p);
    qDebug()<<string ;
    return *this;
}
