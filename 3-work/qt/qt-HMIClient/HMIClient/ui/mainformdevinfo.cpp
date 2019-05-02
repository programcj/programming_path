#include "mainformdevinfo.h"
#include "ui_mainformdevinfo.h"
#include "../hal/am335x_interface/interface.h"

MainFormDevInfo::MainFormDevInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormDevInfo)
{
    ui->setupUi(this);

     gpio_init_out();
}

MainFormDevInfo::~MainFormDevInfo()
{
    delete ui;
}

void MainFormDevInfo::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    uiShowGPIOStatus();
}

void MainFormDevInfo::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void MainFormDevInfo::uiShowGPIOStatus()
{
    QPushButton *btns[]={ui->pushButton_DO_0,
                         ui->pushButton_DO_1,
                         ui->pushButton_DO_2,
                         ui->pushButton_DO_3};
    for(int i=0;i<4;i++){
        int value=gpio_out_read(i);
        if(value==1)
            btns[i]->setStyleSheet("border-radius: 0px; background-color: rgb(215, 255, 171);"
                                   "color: rgb(76, 103, 255); font: 22px;");
        if(value==0)
            btns[i]->setStyleSheet("background-color: rgb(73, 81, 93);"
                                   "color: rgb(255, 255, 255);"
                                   "border-radius: 0px;"
                                   "font: 22px;");
    }
}

void MainFormDevInfo::on_pushButton_DO_0_clicked()
{
    int value=gpio_out_read(0);
    gpio_set_value(0,value==0?1:0);
    uiShowGPIOStatus();
}

void MainFormDevInfo::on_pushButton_DO_1_clicked()
{
    int value=gpio_out_read(1);
    gpio_set_value(1,value==0?1:0);
    uiShowGPIOStatus();
}

void MainFormDevInfo::on_pushButton_DO_2_clicked()
{
    int value=gpio_out_read(2);
    gpio_set_value(2,value==0?1:0);
    uiShowGPIOStatus();
}

void MainFormDevInfo::on_pushButton_DO_3_clicked()
{
    int value=gpio_out_read(3);
    gpio_set_value(3,value==0?1:0);
    uiShowGPIOStatus();
}


