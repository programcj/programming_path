#include "dialognetseteth0.h"
#include "ui_dialognetseteth0.h"
#include <QNetworkInterface>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include "dialoginput.h"
#include "dialogtoast.h"
#include "../public.h"
// /lib/systemd/network/eth0.network

DialogNetSetEth0::DialogNetSetEth0(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNetSetEth0)
{
    ui->setupUi(this);
    m_dhcpTimerId=0;

    ui->lineEdit_MyIP->installEventFilter(this);
    ui->lineEdit_MyMask->installEventFilter(this);
    ui->lineEdit_MyGateway->installEventFilter(this);

#ifdef AM335X
    QSettings *configIniWrite = new QSettings("/lib/systemd/network/eth0.network", QSettings::IniFormat);
    QString Address=configIniWrite->value("Network/Address").toString();
    QString Gateway= configIniWrite->value("Network/Gateway").toString();

    delete configIniWrite;
    if(Address.indexOf('/')>0)
        Address=Address.left(Address.indexOf('/'));
    ui->lineEdit_MyIP->setText(Address);
    ui->lineEdit_MyGateway->setText(Gateway);
#endif
#ifdef Q_OS_LINUX_WEIQIAN
    QNetworkInterface interface=QNetworkInterface::interfaceFromName("eth0");
    QList<QNetworkAddressEntry> entryList = interface.addressEntries();
    //获取IP地址条目列表，每个条目中包含一个IP地址，一个子网掩码和一个广播地址
    foreach(QNetworkAddressEntry entry,entryList) //遍历每一个IP地址条目
    {
        if(entry.ip().protocol()==QAbstractSocket::IPv4Protocol){
            ui->lineEdit_MyIP->setText(entry.ip().toString());
            ui->lineEdit_MyMask->setText(entry.netmask().toString());
            ui->lineEdit_MyGateway->setText("192.168.1.1");
        }
        qDebug()<<"IP Address: "<<entry.ip().toString(); //IP地址
        qDebug()<<"Netmask: "<<entry.netmask().toString(); //子网掩码
        qDebug()<<"Broadcast: "<<entry.broadcast().toString(); //广播地址
    }
#endif
}

DialogNetSetEth0::~DialogNetSetEth0()
{
    delete ui;
}

bool DialogNetSetEth0::eventFilter(QObject *obj, QEvent *event)
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

void DialogNetSetEth0::closeEvent(QCloseEvent *e)
{
    QProcess::startDetached("killall udhcpc");
}

void DialogNetSetEth0::timerEvent(QTimerEvent *e)
{
    QNetworkInterface interface=QNetworkInterface::interfaceFromName("eth0");
    QList<QNetworkAddressEntry> entryList = interface.addressEntries();
    //获取IP地址条目列表，每个条目中包含一个IP地址，一个子网掩码和一个广播地址
    foreach(QNetworkAddressEntry entry,entryList) //遍历每一个IP地址条目
    {
        if(entry.ip().protocol()==QAbstractSocket::IPv4Protocol){
            ui->lineEdit_MyIP->setText(entry.ip().toString());
            ui->lineEdit_MyMask->setText(entry.netmask().toString());
        }
        qDebug()<<"IP Address: "<<entry.ip().toString(); //IP地址
        qDebug()<<"Netmask: "<<entry.netmask().toString(); //子网掩码
        qDebug()<<"Broadcast: "<<entry.broadcast().toString(); //广播地址
    }
    QProcess::startDetached("killall udhcpc");
}

void DialogNetSetEth0::on_bt_save_netconfig_clicked()
{
    //-/lib/systemd/network/eth0.network
    //    [Match]
    //    Name=eth0
    //
    //    [Network]
    //    Address=192.168.3.12/24
    //    Gateway=192.168.3.10
    QString address=ui->lineEdit_MyIP->text();
    QString gateway=ui->lineEdit_MyGateway->text();
    QString netmask=ui->lineEdit_MyMask->text();
    QHostAddress test;

    if (!test.setAddress(address)){
        DialogToast::make("IP 地址格式错误",1000);
        ui->lineEdit_MyIP->setText("");
        return;
    }
    if (!test.setAddress(gateway)){
        DialogToast::make("网关 地址格式错误",1000);
        ui->lineEdit_MyGateway->setText("");
        return;
    }
    if (!test.setAddress(netmask)){
        //QMessageBox::information(this, tr("错误"), tr("地址格式错误"));
        DialogToast::make("子网掩码 地址格式错误",1000);
        ui->lineEdit_MyMask->setText("");
        return;
    }
    {
        QStringList list;
        list<<"eth0"<<address;
        if(netmask.length()>0)
            list<<"netmask"<<netmask;
        QProcess::execute("ifconfig",list);
    }
    {
        QStringList list;
        list<<"add"<<"default"<<"gw"<<gateway;
        QProcess::execute("route",list);
    }
#ifdef Q_OS_LINUX_WEIQIAN
    {
        QFile file("/etc/init.d/net_sh");
        if(file.exists()){
            file.remove();
        }
        if(file.open(QIODevice::ReadWrite | QIODevice::Text )) {
            QTextStream in(&file);
            in<<"#!/bin/sh\n";
            in<<"ifconfig eth0 "<<address<<" netmask "<<netmask<<"\n";
            in<<"route add default gw "<<gateway<<"\n";
            file.close();
        }
        QProcess::execute("chmod +x /etc/init.d/net_sh");
    }
#endif
#ifdef  AM335X
    address=address+"/24";
    QSettings *configIniWrite = new QSettings("/lib/systemd/network/eth0.network", QSettings::IniFormat);
    configIniWrite->setValue("Match/Name", "eth0");
    configIniWrite->setValue("Network/Address", address);
    configIniWrite->setValue("Network/Gateway", gateway);
    delete configIniWrite;
#endif
    if(m_dhcpTimerId!=0){
        killTimer(m_dhcpTimerId);
        m_dhcpTimerId=0;
    }
    accept();
}

void DialogNetSetEth0::on_bt_cancel_netconfig_clicked()
{
    if(m_dhcpTimerId!=0){
        killTimer(m_dhcpTimerId);
        m_dhcpTimerId=0;
    }
    close();
}

void DialogNetSetEth0::on_pushButton_auto_clicked()
{
    QProcess::startDetached("killall QtDemo");
    QProcess::startDetached("killall udhcpc");
    QProcess::startDetached("udhcpc -i eth0");
    if(m_dhcpTimerId!=0) {
        killTimer(m_dhcpTimerId);
    }
    m_dhcpTimerId=startTimer(5000);
}
