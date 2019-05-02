#ifndef DIALOGNETSETETH0_H
#define DIALOGNETSETETH0_H

#include <QDialog>

namespace Ui {
class DialogNetSetEth0;
}

class DialogNetSetEth0 : public QDialog
{
    int m_dhcpTimerId;

    Q_OBJECT
public:
    explicit DialogNetSetEth0(QWidget *parent = 0);
    ~DialogNetSetEth0();

private slots:
    void on_bt_save_netconfig_clicked();

    void on_bt_cancel_netconfig_clicked();

    void on_pushButton_auto_clicked();

private:
    virtual bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent *e);
    virtual void timerEvent(QTimerEvent *e);

    Ui::DialogNetSetEth0 *ui;
};

#endif // DIALOGNETSETETH0_H
