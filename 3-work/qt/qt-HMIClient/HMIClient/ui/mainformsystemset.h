#ifndef MAINFORMSYSTEMSET_H
#define MAINFORMSYSTEMSET_H

#include <QWidget>

namespace Ui {
class MainFormSystemSet;
}

class MainFormSystemSet : public QWidget
{
    Q_OBJECT

public:
    explicit MainFormSystemSet(QWidget *parent = 0);
    ~MainFormSystemSet();

private slots:
    void on_btExitApp_clicked();

    void on_pushButton_save_mqtt_clicked();

    void on_pushButton_cancel_mqtt_clicked();

    void on_pushButton_net_eth_clicked();

    void on_pushButton_net_wlan_clicked();

    void on_pushButton_SaveKangUart_clicked();

private:

    virtual bool eventFilter(QObject *obj, QEvent *event);

    virtual void showEvent(QShowEvent *event);
    virtual void hideEvent(QHideEvent *event);
    Ui::MainFormSystemSet *ui;
};

#endif // MAINFORMSYSTEMSET_H
