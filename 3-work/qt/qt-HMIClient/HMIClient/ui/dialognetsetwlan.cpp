#include "dialognetsetwlan.h"
#include "ui_dialognetsetwlan.h"

DialogNetSetWlan::DialogNetSetWlan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNetSetWlan)
{
    ui->setupUi(this);
}

DialogNetSetWlan::~DialogNetSetWlan()
{
    delete ui;
}

void DialogNetSetWlan::on_bt_cancel_netconfig_clicked()
{
    close();
}
