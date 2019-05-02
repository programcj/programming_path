#ifndef FUN21_H
#define FUN21_H

#include <QDialog>

namespace Ui {
class Fun21;
}
/**
 * 换模
 */
class Fun21: public QDialog {
Q_OBJECT

public:
	explicit Fun21(QWidget *parent = 0);
	~Fun21();

private:
	Ui::Fun21 *ui;
};

#endif // FUN21_H
