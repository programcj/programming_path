#ifndef MESMESSAGE_H
#define MESMESSAGE_H

#include <QDialog>
#include <QtGui>
#include "toastdialog.h"

namespace Ui
{
class MESMessageDlg;
}

class MESMessageDlg: public QDialog
{
Q_OBJECT

public:
	explicit MESMessageDlg(QWidget *parent = 0);
	~MESMessageDlg();

	void setTitle(const QString &title);
	void setText(const QString &text);

	void addButton(QAbstractButton *button, QDialogButtonBox::ButtonRole role);
	QPushButton *addButton(const QString &text,
			QDialogButtonBox::ButtonRole role);
	QPushButton *addButton(QDialogButtonBox::StandardButton button);

	QAbstractButton *clickedButton();

	static void about(QWidget *parent, const QString &title,
			const QString &text);

	static QDialogButtonBox::StandardButton information(QWidget *parent,
			const QString &title, const QString &text, int sbutton,
			QDialogButtonBox::StandardButton def);
	//static MESMessageDlg toast(QWidget *parent,const QString &text,int times);

private slots:

	void on_buttonBox_clicked(QAbstractButton *button);

private:
	void updateSize();
	bool autoAddOkButton;
	QAbstractButton *_clickedButton;
	Ui::MESMessageDlg *ui;
};

#endif // MESMESSAGE_H