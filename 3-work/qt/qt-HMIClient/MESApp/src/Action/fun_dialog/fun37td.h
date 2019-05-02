#ifndef FUN37TD_H
#define FUN37TD_H

/********
 * Fun37 µ÷µ¥
 */
#include <QDialog>
#include <QtGui>

namespace Ui {
class fun37td;
}

class Fun37td : public QDialog
{
    Q_OBJECT
    
public:
    explicit Fun37td(QWidget *parent = 0);
    ~Fun37td();
    
private slots:
    void on_btExit_clicked();

    void on_btMoveUp_clicked();

    void on_btMoveDown_clicked();

    void on_btSave_clicked();

    void on_tableWidget_Order_itemClicked(QTableWidgetItem *item);

private:
    Ui::fun37td *ui;
};

#endif // FUN37TD_H
