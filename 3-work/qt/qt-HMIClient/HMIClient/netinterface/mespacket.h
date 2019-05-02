#ifndef MESPACKET_H
#define MESPACKET_H

#include <QString>
#include <QtCore>
#include <QDataStream>

//Tag ------------------------
#define TAG_TYPE_BIN        0x01 //1	二进制	0x01
#define TAG_TYPE_JSON       0x02 //2	Json	0x02
#define TAG_TYPE_XML        0x03 //3	Xml	0x03
#define TAG_TYPE_BIN_ENCRYPT    0x11    //4	加密二进制	0x11
#define TAG_TYPE_JSON_ENCRYPT   0x12    //5	加密Json	0x12
#define TAG_TYPE_XML_ENCRYPT    0x13    //6	加密Xml	0x13
#define TAG_TYPE_HEART_BEAT     0xA0    //7	心跳	0xA0	心跳数据值上传包体即可。
#define TAG_TYPE_AUT            0xB0    //8	鉴权	0xB0	数据上下行用同一个tag

//包体数据类型
#define PACKET_BODY_TYPE_NULL   0x00    //01	NULL	0x00	0
#define PACKET_BODY_TYPE_INT8   0x01    //02	Int8	0x01	1
#define PACKET_BODY_TYPE_INT16  0x02    //03	Int16	0x02	2
#define PACKET_BODY_TYPE_INT32  0x03    //04	Int32	0x03	4
#define PACKET_BODY_TYPE_INT64  0x04    //05	Int64	0x04	8
#define PACKET_BODY_TYPE_UINT8  0x11    //06	Uint8	0x11	1
#define PACKET_BODY_TYPE_UINT16 0x12    //07	Uint16	0x12	2
#define PACKET_BODY_TYPE_UINT32 0x13    //08	Uint32	0x13	4
#define PACKET_BODY_TYPE_UINT64 0x14    //09	Uint64	0x14	8
#define PACKET_BODY_TYPE_FLOAT  0x20    //10    Float	0x20	4
#define PACKET_BODY_TYPE_DOUBLE 0x21    //11	Double	0x21	8
#define PACKET_BODY_TYPE_STRING 0x30    //12	String	0x30	n
#define PACKET_BODY_TYPE_JSON   0x40    //13	Json	0x40	n
#define PACKET_BODY_TYPE_KLV    0x50    //14	KLV     0x50	n
#define PACKET_BODY_TYPE_BIN    0x60    //15	二进制	0x60	n
#define PACKET_BODY_TYPE_BOOL   0x05    //16	Bool	0x05	1	基本数据类型（固定长度)

#include <QByteArray>
#include "../public.h"

class MESPacketBody {
public:
    quint16 key; //    1	Key	数据ID	2	上传与下发设置为同一个key
    quint16 length; //    3	Length	Value的长度 	2	Type为基本类型该字段可以省略
    QByteArray data;

    MESPacketBody(){
        key=0;
        length=0;
    }

    MESPacketBody(unsigned short key,QJsonObject &json);

    MESPacketBody(unsigned short key,QString &str);

    void setData(QJsonObject &json) {
       data=QJsonDocument(json).toJson();
       length=data.length();
    }

    void setData(QString &str) {
       data=str.toLatin1();
       length=data.length();
    }
};

//一个数据包
class MESPacket
{
public:
    quint8 versions; //    1	 Versions	版本号	1
    quint8 tag; //    2	Tag	包类型	1	具体定义见tag表
    quint16 length; //    3	Length	包体长度	4

    //    4	Value	包体	n
   /// QByteArray body; //须要解成采集点

    MESPacketBody body;

    MESPacket();

	static bool toMESPacket(QByteArray &msg, MESPacket &packet){
        QDataStream stream(&msg, QIODevice::ReadWrite);
		//后端协议处理
        stream >> packet.versions;  //1
        stream >> packet.tag;   //1
        stream >> packet.length;  //4
        stream >> packet.body.key;  //2
        stream >> packet.body.length; //2

        packet.body.data.append(msg.mid(1+1+2+2+2));
		return true;
	}

//    bool toBody(MESPacketBody &mbody){
//        unsigned short key; //    1	Key	数据ID	2	上传与下发设置为同一个key
//        unsigned short length; //    3	Length	Value的长度 	2	Type为基本类型该字段可以省略

//        if(body.length()==0)
//            return false;

//        memcpy(&key, body.data(),2);
//        //memcpy(&length, body.data()+2,2);
//		length = (body[2] & 0x00FF) << 8 ;
//		length |= (body[3] & 0x00FF);
//        if(length!=body.length()-4)
//            return false;
//        mbody.key=key;
//        mbody.length=length;
//        mbody.data.append(body.data()+4,length);
//        return true;
//    }

    void setBody(MESPacketBody &mbody);

    QByteArray toByteArray() const {
        QByteArray array;
        QDataStream stream(&array, QIODevice::ReadWrite);
        quint16 blength =body.data.length();
        quint16  hlength =body.length+4;

        stream << versions;
        stream << tag;
        stream << hlength;
        stream << body.key;
        stream << blength;

        stream.writeRawData(body.data.data(), blength);
        return array;
    }

//    static QByteArray build(unsigned char tag,unsigned short key,QString &data) {
//            QByteArray array;
//            QDataStream stream(&array, QIODevice::ReadWrite);
//            unsigned char ver=0;
//            unsigned long length=0;
//            unsigned short dlen=0;
//            QByteArray dataArray=data.toLatin1();
//            dlen=dataArray.length();
//            length=dlen+4;

//            stream.writeRawData((char*)&ver,1);
//            stream.writeRawData((char*)&tag,1);
//            stream.writeRawData((char*)&length,4);
//            stream.writeRawData((char*)&key,2);
//            stream.writeRawData((char*)&dlen,2);
//            stream.writeRawData(dataArray.data(), dlen);
//            return array;
//    }
};

#endif // MESPACKET_H
