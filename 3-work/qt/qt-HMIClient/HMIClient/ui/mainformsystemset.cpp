#include "mainformsystemset.h"
#include "ui_mainformsystemset.h"
#include "../appconfig.h"
#include "../application.h"
#include "ui/dialoginput.h"
#include "dialogtoast.h"
#include "dialognetseteth0.h"
#include "dialognetsetwlan.h"

MainFormSystemSet::MainFormSystemSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainFormSystemSet)
{
    ui->setupUi(this);

    ui->lineEdit_mqttUrl->installEventFilter(this);
    ui->lineEdit_mqttUser->installEventFilter(this);
    ui->lineEdit_mqttPass->installEventFilter(this);
    ui->lineEdit_MySN->installEventFilter(this);

#ifdef Q_OS_LINUX_WEIQIAN
    ui->comboBox_KangUart->addItem("/dev/ttyAMA0");
    ui->comboBox_KangUart->addItem("/dev/ttyAMA1");
#endif
#ifdef AM335X
    ui->comboBox_KangUart->addItem("/dev/ttyO2");
    ui->comboBox_KangUart->addItem("/dev/ttyO1");
#endif
}

MainFormSystemSet::~MainFormSystemSet()
{
    delete ui;
}

bool MainFormSystemSet::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type()==QEvent::MouseButtonPress) {
        if(obj->inherits("QLineEdit")){
            QLineEdit *line=(QLineEdit*)obj;
            QString text=line->text();
            DialogInput dialog(NULL, DialogInput::none,text,"录入数量"); //弹出输入框输入
            if(dialog.exec() == DialogInput::KeyEnter) {
                line->setText(dialog.text());
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj,event);
}

void MainFormSystemSet::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    qDebug() << "showEvent";

    ui->lineEdit_mqttUrl->setText(AppConfig::getInstance().mqttURL);    
    ui->lineEdit_mqttUser->setText(AppConfig::getInstance().mqttUser);
    ui->lineEdit_mqttPass->setText(AppConfig::getInstance().mqttPassword);

    ui->lineEdit_MySN->setText(AppConfig::getInstance().DeviceSN);


    if(AppConfig::getInstance().serviceModuleKangUart=="/dev/ttyO2")
        ui->comboBox_KangUart->setCurrentIndex(0);

    if(AppConfig::getInstance().serviceModuleKangUart=="/dev/ttyO1")
        ui->comboBox_KangUart->setCurrentIndex(1);

    ui->label_MqttStatusMsg->setText(Application::getInstance()->mesMqttClient->getStatusMsg());
    ui->label_StatusKang->setText(Application::getInstance()->serviceModuleKang->getStatusMessage());
}

void MainFormSystemSet::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void MainFormSystemSet::on_pushButton_save_mqtt_clicked()
{
    AppConfig::getInstance().mqttURL=ui->lineEdit_mqttUrl->text();
    AppConfig::getInstance().mqttUser=ui->lineEdit_mqttUser->text();
    AppConfig::getInstance().mqttPassword=ui->lineEdit_mqttPass->text();
    AppConfig::getInstance().save();

    Application::getInstance()->restartServiceMqttService();

    DialogToast::make("OK",1000);
}

void MainFormSystemSet::on_pushButton_cancel_mqtt_clicked()
{
    ui->lineEdit_mqttUrl->setText(AppConfig::getInstance().mqttURL);
    ui->lineEdit_MySN->setText(AppConfig::getInstance().DeviceSN);
    ui->lineEdit_mqttUser->setText(AppConfig::getInstance().mqttUser);
    ui->lineEdit_mqttPass->setText(AppConfig::getInstance().mqttPassword);
}

void MainFormSystemSet::on_pushButton_net_eth_clicked()
{
    DialogNetSetEth0 dialog;
    dialog.exec();
}

void MainFormSystemSet::on_pushButton_net_wlan_clicked()
{
    DialogNetSetWlan dialog;
    dialog.exec();
}

void MainFormSystemSet::on_pushButton_SaveKangUart_clicked()
{
    QString uart=ui->comboBox_KangUart->currentText();

    AppConfig::getInstance().serviceModuleKangUart=uart;
    AppConfig::getInstance().save();

    Application::getInstance()->restartServiceModleKang();
}

void MainFormSystemSet::on_btExitApp_clicked()
{
    qDebug()<<"开始退出";  
    ::system("/home/asd/linuxdesktop00 -qws &");
    Application::getInstance()->stopAndExit();
}
