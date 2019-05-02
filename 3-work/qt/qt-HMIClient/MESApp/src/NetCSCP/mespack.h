#ifndef MESPACK_H
#define MESPACK_H

#include <QtCore>
#include "mestool.h"

class MESPack
{
	quint8 head;
	quint16 func;
	quint16 length;
	quint16 checkSum;
    quint16 _checkSum; //自己计算出的效验码
	bool checkFlag;
	//命令序列号
	quint8 FDAT_CMDSerial;
	QByteArray body;

	int atPoint;

    Q_ENUMS(FDATResult)
public:

	enum FDATResult
	{
		FDAT_RESULT_NON,		//未定默认
		FDAT_RESULT_SUCESS,	// 0x00 接收成功，并保存 ; 允许下载程序; 注册成功 ;
		FDAT_RESULT_FAILED,	// 0x01	接收数据有问题，不需要重发
		FDAT_RESULT_RESTART,	// 0x02	接收数据有问题，需要重发
		FDAT_RESULT_NOT_PERMISSION, // 0x03	无权限
		FDAT_RESULT_SUCESS_SWING_CARD,	// 0x04	刷卡成功
		FDAT_RESULT_NOT_DOWN_WORK,	// 0x05	没刷下班卡
		FDAT_RESULT_NOT_UP_WORK,	// 0x06	没刷上班卡
		FDAT_RESULT_NON_SWING_CARD,	// 0x07	刷卡未知错误
		FDAT_RESULT_SUCESS_UPFILE,	// 0x08	升级文件成功
		FDAT_RESULT_FAILED_UPFILE,	// 0x09	升级文件失败

		FDAT_RESULT_FAIL_PACK,	//自己定义的，错误的数据包
		FDAT_RESULT_FAIL_CLOSE, //网络己断开
		FDAT_RESULT_FAIL_SEND, //自己定义的，发送出错
		FDAT_RESULT_TIME_OUT,	// 超时

		FDAT_RESULT_REG_OK, 	//注册成功 终端状态正常，允许通讯
		FDAT_RESULT_REG_IP_NON,	//01：IP地址不存在，拒绝通讯
		FDAT_RESULT_REG_NON,	//02：无效
		FDAT_RESULT_REG_FILE_NON,	//03：配置文件不完整

		FDAT_RESULT_FILE_DOWN_ALLOW,	//0x00：允许下载程序
		FDAT_RESULT_FILE_DOWN_VERSION_CMP,	//0x01：软件版本号相同，拒绝下载
		FDAT_RESULT_FILE_DOWN_SPACE_SMALL,	//0x02：FLASH空间太小，不足够存放当前应用程序
		FDAT_RESULT_FILE_DOWN_SERVER_BUSY,	//0x03：服务器忙，请稍后再下载
		FDAT_RESULT_FILE_DOWN_OTHER,	//0x04：其它未知错误

		FDAT_RESULT_FILE_DOWN_DATA,	//	0x00：文件数据
		FDAT_RESULT_FILE_DOWN_ADD_ERR,	//	0x01：该地址非法
        FDAT_RESULT_FILE_DOWN_PAK_ERR, 	//	0x02：包大小非法
	//FDAT_RESULT_FILE_DOWN_LOOP_OTHER, //	0x03：其它未知错误
        FDAT_CHECK_SUM_ERR  //效验出错
	};

	MESPack(int CMDSerial);
	MESPack();

	void setCMDSerial(quint8 cmd);
	quint8 getCMDSerial();

	QByteArray &getBody();

	QByteArray toByteArray();

    bool CheckFlag();
    //数据包里的效验码
    quint16 getCheckSum(){
        return checkSum;
    }
    //计算出的效验码
    quint16 getCheckSumCalc(){
        return _checkSum;
    }

    bool parse(const QByteArray &buff);

	int count()
	{
		return body.length();
	}

	bool isReadEnd()
	{
		return atPoint == body.length();
	}
	quint8 readQUint8()
	{
		quint8 v = (quint8) body[atPoint];
		atPoint += 1;
		return v;
	}

	quint16 readQUint16()
	{
		quint16 v = 0;
        readByte((char*) &v, 2);
		return v;
	}

	quint32 readQUint32()
	{
		quint32 v = 0;
        readByte((char*) &v, 4);
		return v;
	}
    void readByte(void *str, int len)
	{
		QByteArray array = body.mid(atPoint, len);
        memcpy(str, array.data(),len);
		atPoint += len;
	}

	void readToQString(QString &str, int len)
	{
		QByteArray array = body.mid(atPoint, len);
        array.append('\0');
		str = array;
		atPoint += len;
	}

	void readToQByteArray(QByteArray &array, int len)
	{
		array.clear();
		array = body.mid(atPoint, len);
		atPoint += len;
	}

	QDateTime readQDateTime()
	{
		QDateTime time;
		QByteArray array;
		array = body.mid(atPoint, 6);
		time.date().setDate(2000 + array[0], array[1], array[2]);
		time.time().setHMS(array[3], array[4], array[5], 0);
        atPoint += 6;
		return time;
	}
	///////////////////////////////////////
	void writeQString(const QString &str, int len)
	{
		QByteArray arr;
		arr.resize(len);
		arr.fill(0, len);
		arr.replace(0, str.length(), str.toAscii());
		body.append(arr);
	}

	void writeQByteArray(const QByteArray &array)
	{
		body.append(array);
	}

	void writeChars(const char *v, int len)
	{
		body.append(v, len);
	}

	void writeQUint32(quint32 v)
	{
		body.append((char*) &v, 4);
	}
	void writeQUint16(quint16 v)
	{
		body.append((char*) &v, 2);
	}
	void writeQUint8(quint8 v)
	{
		body.append((char) v);
	}

	void writeQDataTime(const QDateTime &dateTime)
	{
		QByteArray v;
		v.resize(6);
		v[0] = dateTime.date().year() % 100;
		v[1] = dateTime.date().month();
		v[2] = dateTime.date().day();
		v[3] = dateTime.time().hour();
		v[4] = dateTime.time().minute();
		v[5] = dateTime.time().second();
		body.append(v);
	}

	void writeQStringIC(const QString &ic)
	{
		char CardID[10];
		memset(CardID, 0, sizeof(CardID));
		MESTool::QStringToICCardID(ic, (unsigned char*) CardID); //	10	HEX	卡号
		body.append(CardID, 10);
	}
};

#endif // MESPACK_H
