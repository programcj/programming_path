#include "toastdialog.h"
#include "ui_toastdialog.h"

ToastDialog::ToastDialog(QWidget *parent) :
		QDialog(parent, Qt::Tool | Qt::FramelessWindowHint), ui(
				new Ui::ToastDialog) {
	ui->setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	//setAttribute(Qt::WA_DeleteOnClose, true);
	animation =0;
}

ToastDialog::ToastDialog(const QString &text, int timems, QWidget *parent) :
		QDialog(parent, Qt::Tool | Qt::FramelessWindowHint), ui(
				new Ui::ToastDialog) {
	ui->setupUi(this);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui->labelText->setText(text);
	m_times = timems;

//使用定时器显示温度
	startTimer(timems);
}

ToastDialog::~ToastDialog() {
	delete ui;
}

void ToastDialog::Toast(QWidget* parent, const QString& text, int timems) {
	ToastDialog *dlg = new ToastDialog(text, timems, parent);
	dlg->setGeometry(800 / 2 - 180, 480 - 100, 360, 80);
	dlg->show();
}

void ToastDialog::timerEvent(QTimerEvent* e) {
	killTimer(e->timerId());
	close();
	delete this;
}
