#include "mestools.h"

#define        QDATETIME_FORMAT_STRING         "yyyy-MM-dd hh:mm:ss:zzz"

QString MESTools::DateTimeToQString(const QDateTime &dateTime)
{
    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

QDateTime MESTools::QStringToDateTime(const QString &dateTimeStr)
{
    return QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
}

QString MESTools::GetCurrentDateTimeStr()
{
    return DateTimeToQString(QDateTime::currentDateTime());
}

bool MESTools::fileIsExists(QString filePath)
{
    QFile file(filePath);
    return file.exists();
}

QString MESTools::jsonToQString(const QJsonObject &json)
{
    QByteArray array=QJsonDocument(json).toJson();
    QByteArray out;
    for(int i=0,j=array.length();i<j;i++){
        if(array[i]==' ')
            continue;
        if(array[i]=='\n')
            continue;
        if(array[i]=='\r')
            continue;
        out.append(array[i]);
    }
    return QString(out);
}

QString MESTools::arrayToHEXString(const QByteArray &array)
{
    QString str;
    for(int i=0,j=array.length();i<j;i++){

    }
    return str;
}
