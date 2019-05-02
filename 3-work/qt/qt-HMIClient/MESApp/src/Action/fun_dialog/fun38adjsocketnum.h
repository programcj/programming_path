#ifndef FUN38ADJSOCKETNUM_H
#define FUN38ADJSOCKETNUM_H

#include <QDialog>
#include "../../Public/public.h"
#include "../tooltextinputdialog.h"
#include "../UIPageButton.h"

namespace Ui {
class Fun38AdjSocketNum;
}

class Fun38AdjSocketNum : public QDialog
{
    Q_OBJECT
    
public:
    explicit Fun38AdjSocketNum(ButtonStatus *btStatus,QWidget *parent = 0);
    ~Fun38AdjSocketNum();
    
private slots:
    void on_btExit_clicked();

    void on_btSave_clicked();

    void on_tableWidget_itemClicked(QTableWidgetItem *item);

private:
    void ShowOrderInfo();
    ButtonStatus *btStatus;
    Ui::Fun38AdjSocketNum *ui;
};

#endif // FUN38ADJSOCKETNUM_H
