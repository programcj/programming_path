#ifndef FUN47PZLR_H
#define FUN47PZLR_H

#include <QDialog>

namespace Ui {
class Fun48pzlr;
}
/**
 * 试料
 */
class Fun48pzlr: public QDialog {
Q_OBJECT

public:
	explicit Fun48pzlr(QWidget *parent = 0);
	~Fun48pzlr();

private:
    Ui::Fun48pzlr *ui;
};

#endif // FUN47PZLR_H
