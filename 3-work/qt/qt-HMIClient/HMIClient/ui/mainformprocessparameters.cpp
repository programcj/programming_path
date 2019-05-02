#include "mainformprocessparameters.h"
#include "ui_mainformprocessparameters.h"
#include "../application.h"

MainFormProcessParameters::MainFormProcessParameters(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormProcessParameters)
{
    ui->setupUi(this);
    connect(Application::getInstance()->serviceDICycle, SIGNAL(sigTotalNumAdd(int,int)),this,SLOT(onTotalNumAdd(int,int)));

    startTimer(1000);
}

MainFormProcessParameters::~MainFormProcessParameters()
{
    delete ui;
}

void MainFormProcessParameters::onTotalNumAdd(int TotalNum, int endmsc)
{
    QLineEdit *totalNums[] = {ui->lineEdit_TotalNum_1,ui->lineEdit_TotalNum_2,ui->lineEdit_TotalNum_3,ui->lineEdit_TotalNum_4};
    QLineEdit *cycles[] = { ui->lineEdit_ProdCycle_1, ui->lineEdit_ProdCycle_2, ui->lineEdit_ProdCycle_3, ui->lineEdit_ProdCycle_4 };

    for(int i=0;i<3;i++){
       QString totalNumText= totalNums[i+1]->text();
       QString cycleText= cycles[i+1]->text();
        totalNums[i]->setText(totalNumText);
        cycles[i]->setText(cycleText);
    }
    totalNums[3]->setText(QString("%1").arg(TotalNum));
    cycles[3]->setText(QString("%1").arg(endmsc));
}

void MainFormProcessParameters::timerEvent(QTimerEvent *event)
{
    QList<int> tempValue=ServiceModuleKang::getInstance()->read_temperature_value(4);

    ui->lineEdit_Temper_1->setText(QString("%1").arg(tempValue[0]));
    ui->lineEdit_Temper_2->setText(QString("%1").arg(tempValue[1]));
    ui->lineEdit_Temper_3->setText(QString("%1").arg(tempValue[2]));
    ui->lineEdit_Temper_4->setText(QString("%1").arg(tempValue[3]));

    QLineEdit *preEdit[]= {ui->lineEdit_P1,ui->lineEdit_P2,ui->lineEdit_P3,ui->lineEdit_P4};
    QList<int> preList=ServiceModuleKang::getInstance()->read_pressure_value(1);
    int v=0;
    if(preList.size()>0){
        v=preList[0];
    }
    for(int i=0;i<3;i++){
        preEdit[i]->setText(preEdit[i+1]->text());
    }
    preEdit[3]->setText(QString("%1").arg(v));
}
