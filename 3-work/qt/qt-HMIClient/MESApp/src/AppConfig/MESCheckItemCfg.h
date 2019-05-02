/*
 * MESCheckItemCfg.h
 *
 *  Created on: 2015年2月2日
 *      Author: cj
 */

#ifndef MESCHECKITEMCFG_H_
#define MESCHECKITEMCFG_H_

#include <QString>
#include <QList>

/*
 *点检项目配置文件
 *版本号：6字节的生成日期(HEX)
 型号总数：1字节（n）
 第1种型号：
 型号名称：XXXX：30字节
 项目类型：0代表“机器”：1字节
 点检项目数量：1字节（M11）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第M11个项目序号：1字节
 第M11个项目：30字节
 项目类型：1代表“模具”：1字节
 点检项目数量：1字节（M12）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第M12个项目序号：1字节
 第M12个项目：30字节
 第2种型号：
 型号名称：XXXX：30字节
 项目类型：0代表“机器”：1字节
 点检项目数量：1字节（M21）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第M21个项目序号：1字节
 第M21个项目：30字节
 项目类型：1代表“模具”：1字节
 点检项目数量：1字节（M22）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第M22个项目序号：1字节
 第M22个项目：30字节

 ……

 第n种型号：
 型号名称：XXXX：30字节
 项目类型：0代表“机器”：1字节
 点检项目数量：1字节（Mn1）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第Mn1个项目序号：1字节
 第Mn1个项目：30字节
 项目类型：1代表“模具”：1字节
 点检项目数量：1字节（Mn2）
 第1个项目序号：1字节
 第1个项目：30字节
 第2个项目序号：1字节
 第2个项目：30字节
 ……
 第Mn2个项目序号：1字节
 第Mn2个项目：30字节

 */
class MESCheckItemCfg {
public:
	class CheckItem {
		int index; //第1个项目序号：1字节
		QString name; //第1个项目：30字节
	public:
		CheckItem() {
			index = 0;
			name = "";
		}
		CheckItem(int index, const char *n) {
			this->index = index;
			this->name = QString(n);
		}

		int getIndex() const {
			return index;
		}

		void setIndex(int index) {
			this->index = index;
		}

		const QString& getName() const {
			return name;
		}

		void setName(const QString& name) {
			this->name = name;
		}
	};

	//点检型号
	class CheckModel {
	public:
		QString ModelName;		//型号名称：XXXX：30字节
		//项目类型：0代表“机器”：1字节
		QList<CheckItem> MachineList;
		//项目类型：1代表“模具”：1字节
		QList<CheckItem> ModeList;
	};

	char version[6]; //版本号：6字节的生成日期(HEX)
	int sumSize; //型号总数：1字节（M）

	QList<CheckModel> List;

    virtual bool write(const QString &path); //配置写入到文件
    virtual bool read(const QString &path); //配置从文件读取
};

#endif /* MESCHECKITEMCFG_H_ */
