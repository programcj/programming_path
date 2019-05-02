#ifndef FUN36TJ_H
#define FUN36TJ_H

/**************************
 *36调机
 */
#include <QtGui>
#include <QDialog>
#include "../../Public/public.h"
#include "../UIPageButton.h"
#include "../toolicscandialog.h"
#include "../tooltextinputdialog.h"

namespace Ui
{
class fun36tj;
}

//MES专用的table录入表每项
class MESTableItem
{
	int v1; //默认值
	int v2; //录入值
	int index; //序号
	QString name; //名称
public:
	MESTableItem()
	{
		index = 0;
		name = "";
		v1 = 0;
		v2 = 0;
	}
	void setIndex(int index)
	{
		this->index = index;
	}
	void setName(const QString &name)
	{
		this->name = name;
	}
	void setValue(int value)
	{
		v1 = value;
	}
	void setInputValue(int value)
	{
		v2 = value;
	}
	const QString &getName()
	{
		return name;
	}
	void Save()
	{
		v1 += v2;
		v2 = 0;
	}
	int getSumValue()
	{
		return v1 + v2;
	}
	int getValue()
	{
		return v1;
	}
	int getInputValue()
	{
		return v2;
	}
};

//MES专用的table录入表
class MESTable
{
public:
	QList<MESTableItem> valueList;
};

/**
 * 调机
 *
 * 南丰: (调机数量)
 * 长虹612:
 * 	调机良品数
 * 	调机次品数
 */
class Fun36tj: public QDialog
{
Q_OBJECT

public:
	explicit Fun36tj(ButtonStatus *btStatus, QWidget *parent = 0);
	~Fun36tj();

	bool Loading();
private slots:
	void on_btExit_clicked();

	void on_btStart_clicked();

	void on_btClose_clicked();

	void on_btSave_clicked();

	void on_btItemUp_clicked();

	void on_btItemNext_clicked();

	void on_tableWidget_itemClicked(QTableWidgetItem *item);

    void on_verticalScrollBar_valueChanged(int value);

private:
    void showEvent(QShowEvent* e);

	void OrderInfoShow();
	int OrderBoyIndex;
	QList<MESTable> m_MESTable;

	ButtonStatus *btStatus;
	Ui::fun36tj *ui;
};

#endif // FUN36TJ_H
