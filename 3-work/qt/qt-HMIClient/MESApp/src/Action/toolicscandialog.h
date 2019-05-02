#ifndef TOOLICSCANDIALOG_H
#define TOOLICSCANDIALOG_H

#include <QtGui>
#include <QDialog>
#include "ui_toolicscandialog.h"
#include "../Public/public.h"
#include "../Server/rfserver.h"

namespace Ui {
class ToolICScanDialog;
}

class ToolICScanDialog: public QDialog {
Q_OBJECT

public:
	explicit ToolICScanDialog(QWidget *parent = 0);
	explicit ToolICScanDialog(const MESPDFuncCfg::SetKeyInfo *keyInfo,
			QWidget *parent = 0);

	~ToolICScanDialog();

	const QString getICCardNo();

    void on_pushButton_2_clicked();
private slots:
	void on_pushButton_clicked();

	void RFIDData(char*icBuff);

    void on_pushButton_3_clicked();

private:
	MESPDFuncCfg::SetKeyInfo m_keyInfo;
	QString icCardNo;
	Ui::ToolICScanDialog *ui;
};

#endif // TOOLICSCANDIALOG_H
