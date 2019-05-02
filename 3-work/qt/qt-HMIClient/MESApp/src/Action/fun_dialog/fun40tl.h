#ifndef FUN40TL_H
#define FUN40TL_H

/**********
 * 40投料
 */
#include <QDialog>
#include <QtGui>
#include "../../Public/public.h"

namespace Ui {
class Fun40tl;
}

class Fun40tl: public QDialog {
Q_OBJECT

public:
	explicit Fun40tl(const QString &icCardId, QWidget *parent = 0);
	~Fun40tl();

private slots:
	void on_btExit_clicked();

	void on_btSave_clicked();

    void on_btUp_clicked();

    void on_tableWidgetValue_itemClicked(QTableWidgetItem *item);

    void on_btNext_clicked();

private:
	void ShowOrderInfo();
    void LoadMateralInfo();
    int IndexMaterial;
    QList<Material> materialList;
	QString m_icCardId;
	Ui::Fun40tl *ui;
};

#endif // FUN40TL_H
