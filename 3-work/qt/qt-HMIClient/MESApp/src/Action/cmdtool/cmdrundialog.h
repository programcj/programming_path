#ifndef CMDRUNDIALOG_H
#define CMDRUNDIALOG_H

#include <QtGui/QDialog>
#include "ui_cmdrundialog.h"
#include <QProcess>

class cmdrundialog : public QDialog
{
    Q_OBJECT

public:
    cmdrundialog(QWidget *parent = 0);
    ~cmdrundialog();
    QProcess *cmd ;
    QString output ;
private slots:
    void on_pb_run_clicked();
    void on_pb_stop_clicked();
    void readOutput();
    void on_pb_close_clicked();
private:
    Ui::cmdrundialogClass ui;
};

#endif // CMDRUNDIALOG_H
