#include "mesmessagedlg.h"
#include "ui_mesmessagedlg.h"

void MESMessageDlg::about(QWidget* parent, const QString& title,
		const QString& text)
{
	MESMessageDlg dlg(parent);
	dlg.setTitle(title);
	dlg.setText(text);
	QPushButton *button = dlg.addButton(QDialogButtonBox::Ok);
	button->setText("确定");
	dlg.exec();
}

QDialogButtonBox::StandardButton MESMessageDlg::information(QWidget* parent,
		const QString& title, const QString& text, int sbutton,
		QDialogButtonBox::StandardButton def)
{
	MESMessageDlg dlg(parent);
	dlg.setTitle(title);
	dlg.setText(text);
	if ((sbutton & (long) QDialogButtonBox::Ok) != 0)
	{
		QPushButton *button = dlg.addButton(QDialogButtonBox::Ok);
		button->setText("确定");
	}
	if ((sbutton & (long) QDialogButtonBox::Yes) != 0)
	{
		QPushButton *button = dlg.addButton(QDialogButtonBox::Yes);
		button->setText("确定");
	}
	if ((sbutton & (long) QDialogButtonBox::No) != 0)
	{
		QPushButton *button = dlg.addButton(QDialogButtonBox::No);
		button->setText("取消");
	}
	if ((sbutton & (int) QDialogButtonBox::Cancel) != 0)
	{
		QPushButton *button = dlg.addButton(QDialogButtonBox::Cancel);
		button->setText("取消");
	}
	if ((sbutton & (int) QDialogButtonBox::Close) != 0)
	{
		QPushButton *button = dlg.addButton(QDialogButtonBox::Close);
		button->setText("关闭");
	}
	return (QDialogButtonBox::StandardButton) dlg.exec();
}

MESMessageDlg::MESMessageDlg(QWidget *parent) :
		QDialog(parent,
				Qt::Tool | Qt::FramelessWindowHint | Qt::WindowCloseButtonHint), ui(
				new Ui::MESMessageDlg)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui->labelTitle->setText("");
	autoAddOkButton = true;
	_clickedButton = 0;
	ui->buttonBox->setCenterButtons(
			style()->styleHint(QStyle::SH_MessageBox_CenterButtons, 0, this));
}

MESMessageDlg::~MESMessageDlg()
{
	delete ui;
}

void MESMessageDlg::setTitle(const QString &title)
{
	ui->labelTitle->setText(title);
}

void MESMessageDlg::setText(const QString &text)
{
	ui->contentText->setText(text);
}

void MESMessageDlg::addButton(QAbstractButton* button,
		QDialogButtonBox::ButtonRole role)
{
	ui->buttonBox->addButton(button, role);
}

QAbstractButton* MESMessageDlg::clickedButton()
{
	return _clickedButton;
}

QPushButton* MESMessageDlg::addButton(const QString& text,
		QDialogButtonBox::ButtonRole role)
{
	QPushButton *pushButton = new QPushButton(text,this);
	addButton(pushButton, role);
	updateSize();
	return pushButton;
}

QPushButton* MESMessageDlg::addButton(QDialogButtonBox::StandardButton button)
{
	QPushButton *pushButton = ui->buttonBox->addButton(button);
	if (pushButton)
		autoAddOkButton = false;
	return pushButton;
}

void MESMessageDlg::on_buttonBox_clicked(QAbstractButton *button)
{
	_clickedButton = button;
	int ret = ui->buttonBox->standardButton(button);
	if (ret == QMessageBox::NoButton)
		ret = -1;
	done(ret);
}

void MESMessageDlg::updateSize()
{
}
