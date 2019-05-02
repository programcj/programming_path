#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../tools/mestools.h"
#include "../netinterface/mesmsghandler.h"

#include<sys/types.h>
#include<signal.h>
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mainFormFirst = new MainFormFirst(this);
    mainFormOrderManagement =new MainFormOrderManagement;
    mainFormBadpcs = new MainFormBadpcs;
    mainFormDevInfo = new MainFormDevInfo;
    mainFormProcessParameters = new MainFormProcessParameters;
    mainFormSystemSet =new MainFormSystemSet;

    ui->stackedWidget->addWidget(mainFormFirst);
    ui->stackedWidget->addWidget(mainFormOrderManagement);
    ui->stackedWidget->addWidget(mainFormBadpcs);
    ui->stackedWidget->addWidget(mainFormDevInfo);
    ui->stackedWidget->addWidget(mainFormProcessParameters);
    ui->stackedWidget->addWidget(mainFormSystemSet);
    ui->stackedWidget->setCurrentIndex(0);

    changleMainPage(0);

    startTimer(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_bt_main_page1_clicked()
{
    changleMainPage(0);
}

void MainWindow::on_bt_main_page2_clicked()
{
    changleMainPage(1);
}

void MainWindow::on_bt_main_page3_clicked()
{
    changleMainPage(2);
}

void MainWindow::on_bt_main_page4_clicked()
{
    changleMainPage(3);
}

void MainWindow::on_bt_main_page5_clicked()
{
    changleMainPage(4);
}

void MainWindow::on_bt_main_page6_clicked()
{
    changleMainPage(5);
}

void MainWindow::timerEvent(QTimerEvent *)
{
    QString curTime=MESTools::GetCurrentDateTimeStr();
    ui->label_TimeText->setText(curTime);
    if(MESMsgHandler::getInstance()->isConnected())
        ui->uiNetConnectFlag->setStyleSheet("border-image: url(:/qrc/res/Server_status.png);");
    else
        ui->uiNetConnectFlag->setStyleSheet("border-image: url(:/qrc/res/Server_status_close.png);");
    //monitor
    {
        QFile file("/tmp/hmiservice.pid");
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString line=stream.readLine();
            file.close();
            int id=line.toInt();
            if(id>0)
                kill((pid_t)id,SIGUSR1);
        }
    }
}

void MainWindow::changleMainPage(int index)
{
    QPushButton *button[]={
        ui->bt_main_page1,
        ui->bt_main_page2,
        ui->bt_main_page3,
        ui->bt_main_page4,
        ui->bt_main_page5,
        ui->bt_main_page6
    };
    for(int i=0;i<6;i++){
        if(i!=index)
            button[i]->setChecked(false);
        else
            button[i]->setChecked(true);
    }
    if(index>=ui->stackedWidget->count())
        return;
    ui->stackedWidget->setCurrentIndex(index);
}
