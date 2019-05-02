#ifndef FUN24FSGZ_H
#define FUN24FSGZ_H

#include <QtGui>
#include <QDialog>
#include "../UIPageButton.h"
#include "../../Public/public.h"

namespace Ui {
class Fun24fsgz;
}

//故障类型的操作须要保存到配置文件中
//故障序号 维修项目,处理状态(NG/OK  未完成/完成), 预计完成时间
class FaultItem: public AbsPropertyBase {
public:
	int index;	//序号
	QString name;
	QString status; //NG/OK  待维修确认/完成
	QString okTime; //
	FaultItem() {
		index = 0;
		name = "";
		status = "";
		okTime = "";
	}
	virtual const char *getClassName() const {
		return "FaultItem";
	}

private:
	virtual void onBindProperty() {
		addProperty("index", Property::AsInt, &index);
		addProperty("name", Property::AsQStr, &name);
		addProperty("status", Property::AsQStr, &status);
		addProperty("okTime", Property::AsQStr, &okTime);
	}
};

/**
 * 辅设故障 机器故障 模具故障
 */
class Fun24fsgz: public QDialog {
Q_OBJECT
	ButtonStatus *btStatus;
public:
	explicit Fun24fsgz(ButtonStatus *bt,
			QWidget *parent = 0);
	~Fun24fsgz();

	void loading();
	void show();
private slots:

	void on_btExit_clicked();

	void on_btStart_clicked();

	void on_btEnd_clicked();

	void on_tableWidget_2_itemClicked(QTableWidgetItem *item);

    void on_btSaveTime_clicked();

    void on_btPlus1_clicked();

    void on_btSub1_clicked();

private:    
    void saveTmpFaultNo(int no);
    int getTmpFaultNo();

    void saveTmpTime(int v);
    int getTmpTime();

    void saveIC(const QString &ic);
    QString getIC();

	QList<FaultItem> m_list;
	Ui::Fun24fsgz *ui;
};

#endif // FUN24FSGZ_H
