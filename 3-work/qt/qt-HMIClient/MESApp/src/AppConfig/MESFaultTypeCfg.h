/*
 * MESFaultTypeCfg.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESFAULTTYPECFG_H_
#define MESFAULTTYPECFG_H_

#include "../Unit/MESLog.h"
#include <QtCore>
#include <QString>

/*
 *故障类型配置文件
 */
class MESFaultTypeCfg {
public:
	class TypeItem {
	public:
		int Index; //故障序号：1字节
		QString Type; //故障类型：30字节

		TypeItem(int index, const char *typeName) {
			this->Index = index;
			this->Type = QString(typeName);
		}

	};

	class TypeInfo {
		QString typeName; //故障名称：机器故障：30字节
		int typeSize; //故障类型数量：1字节（M1）
		QList<TypeItem> TypeList; //故障类型列表
	public:
		QList<TypeItem>& getTypeList() {
			return TypeList;
		}

		const QString& getTypeName() const {
			return typeName;
		}

		void setTypeName(const QString& typeName) {
			this->typeName = typeName;
		}

		int getTypeSize() const {
			return typeSize;
		}

		void setTypeSize(int typeSize) {
			this->typeSize = typeSize;
		}
	};

	char version[6]; //版本号：6字节的生成日期(HEX)
	int sumSize; //故障总数：1字节（n）
	QList<TypeInfo> FaultInfoList; //故障信息

	TypeInfo *getFault(const QString &typeName) {
		int size = FaultInfoList.size();
		for (int i = 0; i < size; i++) {
			if (FaultInfoList[i].getTypeName() == typeName) {
				return &FaultInfoList[i];
			}
		}
		return NULL;
	}

	void Debug();

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESFAULTTYPECFG_H_ */
