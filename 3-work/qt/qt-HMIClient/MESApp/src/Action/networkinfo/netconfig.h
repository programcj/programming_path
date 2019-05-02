#ifndef NETCONFIG_H
#define NETCONFIG_H

#include <QtGui/QDialog>
#include "ui_netconfig.h"
#include <QProcess>
#include "interface.h"
#include "qipaddressedit.h"
#include "qipaddressedititem.h"

class netconfig : public QDialog
{
    Q_OBJECT

public:
    netconfig(QWidget *parent = 0);
    ~netconfig();
    QList<Interface*> ints;
    QProcess *proc;
    bool flag;

public slots:
    void on_sel_changed(const QString &text);
  //  void on_toggled(bool b);
    void on_bt_ok_clicked();
    void refreshInterfaces();
    void readConfigs();
    void writeConfigs();
    void state(bool dhcp);
    void proc_finished(int code);
   // void on_pbtest_clicked();
protected:
    void closeEvent(QCloseEvent * evt);
    void moveEvent(QMoveEvent *);
    void resizeEvent(QResizeEvent *);

private:
    Ui::netconfigClass ui;
};

#endif // NETCONFIG_H
