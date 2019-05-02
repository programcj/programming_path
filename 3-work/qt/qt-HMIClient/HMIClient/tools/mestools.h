#ifndef MESTOOLS_H
#define MESTOOLS_H

#include <QtCore>
#include <QDateTime>
#include "../public.h"

class MESTools
{
public:

    static QString DateTimeToQString(const QDateTime &dateTime);

    static QDateTime QStringToDateTime(const QString &dateTimeStr);

    static QString GetCurrentDateTimeStr();

    static bool fileIsExists(QString filePath);

    static QString jsonToQString(const QJsonObject &json);

    static QString arrayToHEXString(const QByteArray &array);

};

#endif // MESTOOLS_H
