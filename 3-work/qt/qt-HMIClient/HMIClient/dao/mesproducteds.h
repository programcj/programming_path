#ifndef MESPRODUCTEDS_H
#define MESPRODUCTEDS_H

#include <QString>
#include <QtCore>
#include "../public.h"
#include "sqlitebasehelper.h"

class MESProducteds
{

public:
    int ID;
    int TagType;
    QString Msg;
    QString CreatedTime;

    MESProducteds();

    static void onCreate(sqlite::SQLiteBaseHelper *dbHelp);

    static void onAppendMsg(int tag, QJsonObject &json);

    static int onCountMsg();

    static bool onFirstMsg(MESProducteds &msg);

    static void onDelete(int pk);

};

#endif // MESPRODUCTEDS_H
