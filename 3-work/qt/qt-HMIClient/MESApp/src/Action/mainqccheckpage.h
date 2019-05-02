#ifndef MAINQCCHECKPAGE_H
#define MAINQCCHECKPAGE_H

#include <QtGui>
#include <QWidget>
#include <QTableWidgetItem>
#include <QScrollBar>
#include "../Public/public.h"
#include "toolicscandialog.h"
#include "fun_dialog/fun36tj.h"

namespace Ui {
class MainQCCheckPage;
}
/**
 * 次品上传 品质检验录入( 使用 Order 的每件中的OrderBoy.PCS_BadData 每件产品的次品数据 )
 */
class MainQCCheckPage: public QWidget {
Q_OBJECT

public:
	explicit MainQCCheckPage(QWidget *parent = 0);
	~MainQCCheckPage();

	void Loading();
	void showOrderInfo();
	void showQCListInfo();

private slots:
    void on_tableWidgetQCList_itemClicked(QTableWidgetItem *item);

	void on_btSave_clicked();

	void on_btPageLast_clicked();

	void on_btPageNext_clicked();

	void on_btNextItem_clicked();

	void on_btUpItem_clicked();

	void OnMESConfigRefurbish(const QString &name);
	void OnUpdateMainOrder();
private:
	QList<MESTable> m_MESTable;
	OrderIndex m_OrderIndex;
	Ui::MainQCCheckPage *ui;
};

#endif // MAINQCCHECKPAGE_H
