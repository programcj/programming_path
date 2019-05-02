#ifndef PUSHMSG_H
#define PUSHMSG_H

#include <QObject>

class PushMsg
{
public:
    QString time;
    QString type;
    QString Tag;
    QString TimecardNo;
    QString DeviceNo;
    QString OrderNo;


    PushMsg();


};

#endif // PUSHMSG_H
