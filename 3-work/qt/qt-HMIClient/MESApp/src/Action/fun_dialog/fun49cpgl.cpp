#include "fun49cpgl.h"
#include "ui_fun49cpgl.h"

Fun49cpgl::Fun49cpgl(QWidget *parent) :
		QDialog(parent), ui(new Ui::Fun49cpgl) {
	ui->setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	connect(ui->btExit, SIGNAL(clicked()), this, SLOT(close()));
}

Fun49cpgl::~Fun49cpgl() {
	delete ui;
}
