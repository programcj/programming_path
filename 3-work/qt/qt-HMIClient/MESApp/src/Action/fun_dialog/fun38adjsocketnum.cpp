#include "fun38adjsocketnum.h"
#include "ui_fun38adjsocketnum.h"
#include "../plugin/mesmessagedlg.h"
#include "../toolicscandialog.h"

Fun38AdjSocketNum::Fun38AdjSocketNum(ButtonStatus *btStatus,QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::Fun38AdjSocketNum)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui->tableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

	this->btStatus=btStatus;
	ShowOrderInfo();
}

Fun38AdjSocketNum::~Fun38AdjSocketNum()
{
	delete ui;
}

void Fun38AdjSocketNum::on_btExit_clicked()
{
	close();
}

//已生产良品数大于或等于派工数量，禁止调整模穴数！
void Fun38AdjSocketNum::on_btSave_clicked()
{
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	QList<int> list;
	{
		for (int i = 0; i < ui->tableWidget->rowCount(); i++)
		{
			QTableWidgetItem *item = ui->tableWidget->item(i, 1);
			if (item == NULL)
				return;
			int v = item->text().toInt();
			if (v == 0)
			{
				ToastDialog::Toast(NULL, "录入为0的模次了!", 2000);
				return;
			}
			list.append(v);
		}
	}
	Order &mainOrder = OrderMainOperation::GetInstance().mainOrderCache;
	OtherSetInfo info;
	OtherSetInfo::AdjSock adj;
	adj.DispatchNo = mainOrder.getMoDispatchNo();
	adj.DispatchPrior = mainOrder.getMoDispatchPrior();
	{
		for (int i = 0; i < mainOrder.orderBoyList.size(); i++)
		{
			OtherSetInfo::ItemProduct item;
			item.MO = mainOrder.orderBoyList[i].getMoDispatchNo(); //	20	ASC	工单号
			item.ItemNO = mainOrder.orderBoyList[i].getPcsItemNo(); //	20	ASC	产品编号
			item.DispatchQty = mainOrder.orderBoyList[i].getPcsDispatchQty(); //	4	DWORD	派工数量
			item.SocketNum = list[i]; //	1	HEX	模穴数 [产品模穴]
			adj.productList.append(item);
			//修改模穴
			mainOrder.orderBoyList[i].OnUpdatePcsSocketNum2(list[i]);
		}
	}

	info.setAdjSock(adj);
	Notebook("调整模穴", info).insert();
	///
	{
		//修改主单的派工项次 为原项次+0.01
		//清空模次
		//清空次品记录数据和调机数据
		//次品总数清零
		//派工数量=原来的派工数量-原来已经生产的良品数量
		//每件产品每种次品类型对应的次品数量清零
		// 调机数清零
		// 已生产良品数清零
		// 修改模穴后需要换班操作
		float DispatchingItem = mainOrder.getMoDispatchPrior().toFloat() + 0.01;
		QString str;
		str.sprintf("%.2f", DispatchingItem);
		mainOrder.setMoDispatchPrior(str);
        mainOrder.setMoTotalNum(0);
		for (int i = 0; i < mainOrder.orderBoyList.size(); i++)
		{
			mainOrder.orderBoyList[i].setPcsDispatchQty(
					mainOrder.orderBoyList[i].getPcsDispatchQty()
							- mainOrder.orderBoyList[i].getTotalOkNum());
			mainOrder.orderBoyList[i].setADJOKNum(0);
			mainOrder.orderBoyList[i].setADJEmptyMoldNum(0);
			mainOrder.orderBoyList[i].setAdjDefCount(0);
			mainOrder.orderBoyList[i].setCurClassInspectNo(0);
			mainOrder.orderBoyList[i].setPcsAdjNum(0);
			mainOrder.orderBoyList[i].setCurClassPolishNo(0);
			mainOrder.orderBoyList[i].setTotalOkNum(0);
			mainOrder.orderBoyList[i].setCurClassBadNo(0);
			mainOrder.orderBoyList[i].setCurClassProductNo(0);
		}
		mainOrder.subimt();
		OrderMainOperation::GetInstance().OnUpdate();
	}
	{
		//换单一次
		/**
		 * 换单上传 都是上传的次单的信息头
		 */

		/**********************************************/
		BrushCard brush(mainOrder, ic.getICCardNo(), BrushCard::AsCHANGE_ORDER,
				Tool::GetCurrentDateTimeStr(), 0);
		Notebook("换单开始", brush).insert();

		brush.setIsBeginEnd(1);
		Notebook("换单结束", brush).insert();
	}
	ToastDialog::Toast(NULL, "保存成功!", 2000);
	ShowOrderInfo();
}

void Fun38AdjSocketNum::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
	if (item == NULL)
		return;
	ToolTextInputDialog dialog(NULL, ToolTextInputDialog::typeNumber, "",
			"重新录入模穴");
	if (dialog.exec() == ToolTextInputDialog::KeyHide)
		return;
	int v = dialog.text().toInt();
	if (v == 0)
	{
		ToastDialog::Toast(NULL, "录入0为的模穴!", 2000);
		return;
	}
	QString str;
	str.sprintf("%d", v);
	ui->tableWidget->setItem(item->row(), 1, new QTableWidgetItem(str));
}

void Fun38AdjSocketNum::ShowOrderInfo()
{
	Order &mainOrder = OrderMainOperation::GetInstance().mainOrderCache;
	int len = mainOrder.orderBoyList.size();
	ui->tableWidget->setRowCount(len);
	for (int i = 0; i < len; i++)
	{
		QString str;
		ui->tableWidget->setItem(i, 0,
				new QTableWidgetItem(mainOrder.orderBoyList[i].getPcsItemNo()));
		str.sprintf("%d", mainOrder.orderBoyList[i].getPcsSocketNum2());
		ui->tableWidget->setItem(i, 1, new QTableWidgetItem(str));
	}
}
