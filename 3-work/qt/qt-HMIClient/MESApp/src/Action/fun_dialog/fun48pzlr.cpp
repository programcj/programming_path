#include "fun48pzlr.h"
#include "ui_fun48pzlr.h"

Fun48pzlr::Fun48pzlr(QWidget *parent) :
		QDialog(parent), ui(new Ui::Fun48pzlr) {
	ui->setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	connect(ui->btExit, SIGNAL(clicked()), this, SLOT(close()));
}

Fun48pzlr::~Fun48pzlr() {
	delete ui;
}
