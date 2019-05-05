#include "dialogswingcard.h"
#include <QDesktopWidget>
#include "ui_dialogswingcard.h"

DialogSwingCard::DialogSwingCard(QWidget *parent) :
    QDialog(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(new Ui::DialogSwingCard)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground); //透明处理
    QDesktopWidget *desk=QApplication::desktop();
    int wd=desk->width();
    int ht=desk->height();
    this->move((wd-width())/2,(ht-height())/2);

    rfidRead=new HALRFIDRead();
    CardID="";
    connect(rfidRead, SIGNAL(sig_ReadRFIDUID(QString)),this,SLOT(slotsReadrfiduid(QString)));
    rfidRead->start();
}

DialogSwingCard::~DialogSwingCard()
{
    rfidRead->requestInterruption();
    rfidRead->wait();

    delete rfidRead;
    delete ui;
}

void DialogSwingCard::setTitleText(const QString &text)
{
    ui->label_SubMsg->setText(text);
}

void DialogSwingCard::slotsReadrfiduid(QString uid)
{
    qDebug()<< QString("read rfid uid:%1").arg(uid);
    CardID=uid;
    accept();
}

void DialogSwingCard::on_pushButton_OK_clicked()
{
    //exec（）是一个循环时间函数，哪它什么时候才能返回了？
    //当调用 accept（）（返回QDialog::Accepted
    // reject（）（返回QDialog::Rejected），
    //done（int r）（返回r），close（）（返回QDialog::Rejected），
    //hide（）（返回QDialog::Rejected），
    //destory（）（返回QDialog::Rejected）。
    //还有就是delete 自己的时候也会返回 QDialog::Rejected（destory（）就会delete自己）
    close();
}
