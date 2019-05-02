#include "tempecalibrate.h"
#include <QDebug>
tempecalibrate::tempecalibrate(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui.cb_channel->addItem("1");
	ui.cb_channel->addItem("2");
	ui.cb_channel->addItem("3");
	ui.cb_channel->addItem("4");
	connect(ui.pb_close, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui.pb_ok, SIGNAL(clicked()), this, SLOT(accept()));
}

tempecalibrate::~tempecalibrate()
{

}


QString tempecalibrate::getEditTempValue()
{

  return ui.edittemp->text();
}

int tempecalibrate::getCombChannelValue()
{

  qDebug()<<"currentIndex:" << ui.cb_channel->currentIndex();
  return ui.cb_channel->currentIndex();
}
