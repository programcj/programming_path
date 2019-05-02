#ifndef MESTOOL_H
#define MESTOOL_H

#include <QtCore>

class MESTool
{
public:
    /**
     * 把IC的HEX字符串格式转换成char[6]，可用于协议通信
     */
    static void QStringToICCardID(const QString &icID,unsigned char v[6])
    {
        bool ok;
        //qDebug() << icID;
        quint32 id = icID.toInt(&ok, 16);
        if (ok == false)
        {
            sscanf(icID.toAscii().data(), "%x", &id);
        }
        for (int i = 0; i < 4; i++)
        {
            v[i] = ((char*) &id)[3 - i];
        }
    }

    /*
    * 把DateTime的字符串格式转换成 char[6]中,分别为年月日时分秒，可用于协议通信
    */
   static void QStringTimToCharArray(const QString &dateTimeStr,unsigned char v[6])
   {
       QDateTime dateTime = QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
       v[0] = dateTime.date().year() % 100;
       v[1] = dateTime.date().month();
       v[2] = dateTime.date().day();
       v[3] = dateTime.time().hour();
       v[4] = dateTime.time().minute();
       v[5] = dateTime.time().second();
   }

    /**
     * 把DateTime 转换成 char[6]中,分别为年月日时分秒，可用于协议通信
     */
    static void DateTimeToCharArray(const QDateTime &dateTime,unsigned char v[6])
    {
        v[0] = dateTime.date().year() % 100;
        v[1] = dateTime.date().month();
        v[2] = dateTime.date().day();
        v[3] = dateTime.time().hour();
        v[4] = dateTime.time().minute();
        v[5] = dateTime.time().second();
    }

    static QDateTime QStringToDateTime(const QString &dateTimeStr)
    {
          return QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
    }

    static QDateTime DateTimeFormCharArray(const unsigned char v[6])
    {
        QString dateTimeStr;
        dateTimeStr.sprintf("%d-%02d-%02d %02d:%02d:%02d",v[0]+2000,v[1],v[2],v[3],v[4],v[5]);
        return QDateTime::fromString(dateTimeStr, "yyyy-MM-dd hh:mm:ss");
    }

    //把QByteArray数据包转换成HEX格式的字符串
    static QString CSCPPackToQString(const QByteArray &arr)
    {
        QString str;
        for(int i=0;i<arr.length();i++)
        {
            QString tmp;
            tmp.sprintf("%02X ",(quint8)arr[i]);
            str+=tmp;
        }
        return str;
    }

    //把HEX格式的字符串转换成QByteArray数据包
    static QByteArray CSCPPackToQByteArray(const QString &hex)
    {
        QString str=hex;
        QByteArray array;
        str=str.replace(" ","");
        for(int i=0;i<str.length();i+=2)
        {
            QString item=str.mid(i,2);
            bool flag=false;
            array.append((char)item.toInt(&flag,16));
        }

        return array;
    }
};

#endif // MESTOOL_H
