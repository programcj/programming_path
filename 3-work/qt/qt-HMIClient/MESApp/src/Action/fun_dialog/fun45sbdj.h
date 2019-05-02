#ifndef FUN45SBDJ_H
#define FUN45SBDJ_H

#include <QDialog>
#include "../../Public/public.h"
#include "../toolicscandialog.h"

namespace Ui {
class Fun45sbdj;
}

class MES45Table {
public:
	class Item {
	public:
		int number;
		QString name;
		int value; // 0：NG 1:OK  2:未开机
	};

	QList<Item> List;

	void insert(int number, const QString &name) {
		Item item;
		item.number = number;
		item.name = name;
		item.value = 0;
		List.append(item);
	}

	void setItemValue(int index, int value) {
		List[index].value = value;
	}

};

class Fun45sbdj: public QDialog {
Q_OBJECT

public:
	explicit Fun45sbdj(QWidget *parent = 0);
	~Fun45sbdj();
	bool Loading();

private slots:
	void on_btSave_clicked();

	void on_btNotStart_clicked();

	void on_btExit_clicked();

	void on_comboBox_currentIndexChanged(int index);

	void on_tableWidget_itemClicked(QTableWidgetItem *item);

private:
	void showEvent(QShowEvent* );

	void ShowTableInfo();
	//机器品牌
	QString MachineBrand;
	MES45Table Tab1;
	MES45Table Tab2;

	Ui::Fun45sbdj *ui;
};

#endif // FUN45SBDJ_H
