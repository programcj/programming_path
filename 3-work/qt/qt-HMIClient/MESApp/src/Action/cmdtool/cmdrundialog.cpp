#include "cmdrundialog.h"
#include "../../Public/public.h"
cmdrundialog::cmdrundialog(QWidget *parent ) :
		QDialog(parent) {
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui.lineEdit->setText("ping " + AppInfo::GetInstance().getServerIp());
	cmd = new QProcess(this);
	connect(ui.lineEdit, SIGNAL(returnPressed()), this,
			SLOT(on_pb_run_clicked()));
	//connect(ui.pb_run, SIGNAL(clicked(bool)), this, SLOT(on_pb_run_clicked()));
	connect(ui.pb_stop, SIGNAL(clicked(bool)), this,
			SLOT(on_pb_stop_clicked()));
	connect(cmd, SIGNAL(readyRead()), this, SLOT(readOutput()));
}

cmdrundialog::~cmdrundialog() {
	delete cmd;
}

void cmdrundialog::on_pb_run_clicked() {

	QString input = ui.lineEdit->text();
	logDebug(input);
	cmd->start(input);
	output = "";
	ui.textEdit->setText("");
}

void cmdrundialog::readOutput()
{

	output += cmd->readAll();
	ui.textEdit->setText(output);

}

void cmdrundialog::on_pb_stop_clicked() {
	cmd->write("quit");
	cmd->kill();
}

void cmdrundialog::on_pb_close_clicked() {

	cmd->write("quit");
	cmd->kill();
	close();
}
