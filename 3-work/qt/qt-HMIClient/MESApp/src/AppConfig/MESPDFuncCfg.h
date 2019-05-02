/*
 * MESOrderFunButton.h
 *
 *  Created on: 2015年1月30日
 *      Author: cj
 */

#ifndef MESORDERFUNBUTTON_H_
#define MESORDERFUNBUTTON_H_

#include <QtCore>
#include <QString>
#include <QList>

/*
 *生产工单功能区按键配置文件
 //	pd_func_cfg.bin
 //	不可配置按键总数：1字节（m）（m目前固定为2）
 //	第1按键：
 //	功能序号：2字节 （0：管理员）
 //	功能权限：4字节
 //	状态：1字节        表示：是否停机，注释：0：不停机，1：停机
 //	第2按键：
 //	功能序号：2字节（1：QC巡检）
 //	功能权限：4字节
 //	状态：1字节        表示：是否停机，注释：0：不停机，1：停机
 //
 //	可配置按键总数：1字节（n）
 //	第1按键：
 //	功能名称：8字节
 //	功能序号：2字节
 //	功能权限：4字节
 //	状态：1字节        表示：是否停机，注释：0：不停机，1：停机
 //	第2按键：
 //	功能名称：8字节
 //	功能序号：2字节
 //	功能权限：4字节
 //	状态：1字节        表示：是否停机，注释：0：不停机，1：停机
 //
 //	……
 //
 //	第n按键：
 //	功能名称：8字节
 //	功能序号：2字节
 //	功能权限：4字节
 //	状态：1字节        表示：是否停机，注释：0：不停机，1：停机
 //
 */
class MESPDFuncCfg {
public:
	enum KeyStatus {
		//0：不停机，1：停机
        NOT_STOP=0,
        STOP=1,
        NONE,
	};

	//配置按键信息
	class SetKeyInfo {
	public:
		QString name; //功能名称：8字节,(不可配置按键没有这个)
		quint16 funIndex; //功能序号：2字节
		quint32 funP; //功能权限：4字节
		KeyStatus status;	//状态：1字节 表示：是否停机，注释：0：不停机，1：停机

		SetKeyInfo(const char *name, quint16 funIndex, quint32 funP,
				KeyStatus status) {
			this->name = QString(name);
			this->funIndex = funIndex;
			this->funP = funP;
			this->status = status;
		}
		SetKeyInfo(){
			name="";
			funIndex=0;
			funP=0;
			status=NONE;
		}
        //是否是停机功能
        bool isStopMachineFun(){
            return status==1;
        }
	};

	char version[6]; //版本号：6字节的生成日期(HEX)
	quint8 notSetSize; //不可配置按键总数：1字节（m）（m目前固定为2）
	QList<SetKeyInfo> NotSetKeyList; //不可配置按键信息
	quint8 setSize; //可配置按键总数：1字节（n）
	QList<SetKeyInfo> SetKeyList; //可配置按键信息

	MESPDFuncCfg::SetKeyInfo *getSetKeyInfo(quint16 funIndex);

    void Debug();

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESORDERFUNBUTTON_H_ */
