#ifndef DIALOGNETSETWLAN_H
#define DIALOGNETSETWLAN_H

#include <QDialog>

namespace Ui {
class DialogNetSetWlan;
}

class DialogNetSetWlan : public QDialog
{
    Q_OBJECT

public:
    explicit DialogNetSetWlan(QWidget *parent = 0);
    ~DialogNetSetWlan();

private slots:
    void on_bt_cancel_netconfig_clicked();

private:
    Ui::DialogNetSetWlan *ui;
};

#endif // DIALOGNETSETWLAN_H
