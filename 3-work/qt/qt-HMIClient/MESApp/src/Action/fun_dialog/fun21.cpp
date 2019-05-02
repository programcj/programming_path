#include "fun21.h"
#include "ui_fun21.h"

Fun21::Fun21(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Fun21)
{
    ui->setupUi(this);
}

Fun21::~Fun21()
{
    delete ui;
}
