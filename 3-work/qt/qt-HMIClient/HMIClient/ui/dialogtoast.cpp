#include "dialogtoast.h"
#include "ui_dialogtoast.h"
#include <QDesktopWidget>

DialogToast::DialogToast(QWidget *parent) :
    QDialog(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(new Ui::DialogToast)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground); //透明处理
    QDesktopWidget *desk=QApplication::desktop();
    int wd=desk->width();
    int ht=desk->height();
    this->move((wd-width())/2,(ht-height())/2);
}

DialogToast::DialogToast(QString message, QWidget *parent):
    QDialog(parent, Qt::Tool | Qt::FramelessWindowHint),
    ui(new Ui::DialogToast)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground); //透明处理
    QDesktopWidget *desk=QApplication::desktop();
    int wd=desk->width();
    int ht=desk->height();
    this->move((wd-width())/2,(ht-height())/2);
    ui->label->setText(message);
}

void DialogToast::setMessage(QString &message)
{
    ui->label->setText(message);
}

void DialogToast::show(int interval)
{
    if(interval>0)
        startTimer(interval);
    exec();
}

void DialogToast::make(QString message, int toast)
{
    DialogToast dialog(message);
    dialog.show(toast);
}

DialogToast::~DialogToast()
{
    delete ui;
}

void DialogToast::timerEvent(QTimerEvent *event)
{
    killTimer(event->timerId());
    close();
}
