#include <QMessageBox>
#include "fun37td.h"
#include "ui_fun37td.h"
#include "../../Public/public.h"
#include "../plugin/mesmessagedlg.h"

Fun37td::Fun37td(QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::fun37td)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	connect(ui->btExit, SIGNAL(clicked()), this, SLOT(close()));

	QList<Order> list;
	if (Order::queryAll(list))
	{
        ui->tableWidget_Order->setColumnCount(2);
		ui->tableWidget_Order->setRowCount(list.size());
        ui->tableWidget_Order->setColumnWidth(0, 50);
        ui->tableWidget_Order->setColumnWidth(1, 220);

		ui->tableWidget_Order->setHorizontalHeaderItem(0,
				new QTableWidgetItem("工单列表"));
	}
	for (int i = 0; i < list.size(); i++)
	{
        ui->tableWidget_Order->setItem(i, 0,
                                       new QTableWidgetItem(QString::number(i+1)));
        ui->tableWidget_Order->setItem(i, 1,
				new QTableWidgetItem(list[i].getMoDispatchNo()));
	}

	this->ui->tableWidget_Order->setSelectionBehavior(
			QAbstractItemView::SelectRows);  //单击选择一行
	this->ui->tableWidget_Order->setSelectionMode(
			QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中
	this->ui->tableWidget_Order->setEditTriggers(
			QAbstractItemView::NoEditTriggers);   //设置每行内容不能编辑
    //this->ui->tableWidget_Order->setAlternatingRowColors(true); //设置隔一行变一颜色，即：一灰一白
	ui->tableWidget_Order->horizontalHeader()->setResizeMode(QHeaderView::Fixed);//列表不能移动
	//QHeaderView::ResizeToContents
	ui->tableWidget_Order->horizontalHeader()->setVisible(false);
	ui->tableWidget_Order->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);//去掉水平滚动条
	ui->tableWidget_Order->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

Fun37td::~Fun37td()
{
	delete ui;
}

void Fun37td::on_btExit_clicked()
{
	close();
}

//上移
void Fun37td::on_btMoveUp_clicked()
{
	QList<QTableWidgetSelectionRange> ranges =
			this->ui->tableWidget_Order->selectedRanges();
	int count = ranges.count();
	if (count != 1)
		return;
	int selectItem = ranges.at(0).topRow();
	if (selectItem == 0 || selectItem == 1)
	{
		MESMessageDlg::about(0, "提示", "主单位置不可改变");
		return;
	}
    QString selectNo = ui->tableWidget_Order->item(selectItem, 1)->text();
    QString selectNoUp = ui->tableWidget_Order->item(selectItem - 1, 1)->text();
    ui->tableWidget_Order->item(selectItem, 1)->setText(selectNoUp);
    ui->tableWidget_Order->item(selectItem - 1, 1)->setText(selectNo);
	ui->tableWidget_Order->selectRow(selectItem - 1);
}

//下移
void Fun37td::on_btMoveDown_clicked()
{
	QList<QTableWidgetSelectionRange> ranges =
			this->ui->tableWidget_Order->selectedRanges();
	int count = ranges.count();
	if (count != 1)
		return;
	int selectItem = ranges.at(0).topRow();
	if (selectItem == 0)
	{
		MESMessageDlg::about(0, "提示", "主单位置不可改变");
		return;
	}
	if (selectItem + 1 >= ui->tableWidget_Order->rowCount())
		return;
    QString selectNo = ui->tableWidget_Order->item(selectItem, 1)->text();
	QString selectNoNext =
            ui->tableWidget_Order->item(selectItem + 1, 1)->text();
	if (selectNoNext.length() == 0)
		return;
    ui->tableWidget_Order->item(selectItem, 1)->setText(selectNoNext);
    ui->tableWidget_Order->item(selectItem + 1, 1)->setText(selectNo);
	ui->tableWidget_Order->selectRow(selectItem + 1);
}

void Fun37td::on_btSave_clicked()
{
	QList<OtherSetInfo::AdjOrder> list;
	for (int i = 0; i < ui->tableWidget_Order->rowCount(); i++)
	{
		OtherSetInfo::AdjOrder adj;
        QString no = ui->tableWidget_Order->item(i, 1)->text();
		adj.SerialNumber = i + 1;
		adj.DispatchNo = no;
		adj.DispatchPrior = Order::query(no).getMoDispatchPrior();
		list.append(adj);
        Order::UpdateMainOrderFlag(no,i+1);
	}

	OtherSetInfo myOrder;
	myOrder.setAdjOrderList(list);

	Notebook("调单", myOrder).insert();
	ToastDialog::Toast(NULL,"保存成功",1000);
}

void Fun37td::on_tableWidget_Order_itemClicked(QTableWidgetItem *item)
{
	//显示这个工单的信息
	int OrderIndex = item->row() + 1;
	Order order;
	if (Order::query(order, OrderIndex))
	{
		order.getBoy();
	}
	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;
	OrderMainOperation::OrderToQMap(order, orderMap, 0, orderBoyMap);
	orderMap.unite(orderBoyMap);

	for (int i = 0; i < ui->tableWidget_info->rowCount(); i++)
	{
		QString key = ui->tableWidget_info->item(i, 0)->text();
		QString v = orderMap[key];
		ui->tableWidget_info->setItem(i, 1, new QTableWidgetItem(v));
	}

	ui->tableWidget_info->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
}
