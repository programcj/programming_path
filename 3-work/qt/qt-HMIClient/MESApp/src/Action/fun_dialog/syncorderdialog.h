#ifndef SYNCORDERDIALOG_H
#define SYNCORDERDIALOG_H

#include <QDialog>
#include "../../Public/public.h"

namespace Ui {
class SyncOrderDialog;
}

class SyncOrderDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SyncOrderDialog(QWidget *parent = 0);
    ~SyncOrderDialog();

    QList<MESOtherResponse::OrderIndex> orderIndexList;

public slots:
    void slot_protocol_other_sync_order(const MESOtherResponse& response);

private slots:
    void on_btExit_clicked();
    void _q_time_Start();

private:
    bool flag;
    bool syncSubimt;
    QTimer timer;
    Ui::SyncOrderDialog *ui;
};

#endif // SYNCORDERDIALOG_H
