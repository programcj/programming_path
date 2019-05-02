#ifndef FUN41XJ_H
#define FUN41XJ_H

#include <QDialog>
#include "../../Public/public.h"
#include "../UIPageButton.h"

namespace Ui {
class Fun41xj;
}
/**
 * 巡机 41
 *   是针对每件产品的
 */
class Fun41xj: public QDialog {
Q_OBJECT

public:
	explicit Fun41xj(ButtonStatus *btStatus,
			QWidget *parent = 0);
	~Fun41xj();

private slots:
	void on_btSave_clicked();

	void on_btExit_clicked();

    void on_tableWidgetValue_itemClicked(QTableWidgetItem *item);

private:
	void ShowOrderInfo();
	OrderIndex m_OrderIndex;
	ButtonStatus *btStatus;
	Ui::Fun41xj *ui;
};

#endif // FUN41XJ_H
