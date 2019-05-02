#ifndef FUN42DM_H
#define FUN42DM_H

#include <QDialog>
#include "../../Public/public.h"
#include "../UIPageButton.h"

namespace Ui
{
class Fun42dm;
}

class Fun42dm: public QDialog
{
Q_OBJECT

public:
	explicit Fun42dm(ButtonStatus *btStatus, QWidget *parent = 0);
	~Fun42dm();

private slots:
	void on_btSave_clicked();

	void on_btExit_clicked();

	void on_tableWidgetValue_itemClicked(QTableWidgetItem *item);

private:
	void ShowOrderInfo();
	OrderIndex m_OrderIndex;
	ButtonStatus *btStatus;
	Ui::Fun42dm *ui;
};

#endif // FUN42DM_H
