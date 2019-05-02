#include "inputtime.h"

InputTime::InputTime(QWidget *parent) :
		QDialog(parent) {
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	setWindowModality(Qt::WindowModal);
	connect(ui.pb_close, SIGNAL(clicked()), this, SLOT(reject()));
	connect(ui.pb_ok, SIGNAL(clicked()), this, SLOT(accept()));
	ui.dateTimeEdit->setDateTime(QDateTime::currentDateTime());
}

InputTime::~InputTime() {

}

QString InputTime::getEditTimeText() {
	return ui.dateTimeEdit->text();
}
