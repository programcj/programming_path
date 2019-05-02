#ifndef FUN43ORDERMOVE_H
#define FUN43ORDERMOVE_H

#include <QtGui>
#include <QtGui/QDialog>
#include "ui_fun43ordermove.h"
#include "../../Public/public.h"
#include "../toolicscandialog.h"
#include "../UIPageButton.h"

class Fun43OrderMove: public QDialog {
Q_OBJECT

public:
	Fun43OrderMove(ButtonStatus *btStatus,QWidget *parent = 0);
	~Fun43OrderMove();
	OrderIndex m_orderIndex;

	bool loading();

private slots:
	void on_btOrderUp_clicked();

	void on_btOrderNext_clicked();

	void on_btExit_clicked();

	void on_btOK_clicked();

    void on_tableWidget_2_itemClicked(QTableWidgetItem *item);

private:
	void showOrderTableInfo();
	QString myMachineNo; //自己的机器编号
	ButtonStatus *btStatus;
	Ui::Fun43OrderMoveClass ui;
};

#endif // FUN43ORDERMOVE_H
