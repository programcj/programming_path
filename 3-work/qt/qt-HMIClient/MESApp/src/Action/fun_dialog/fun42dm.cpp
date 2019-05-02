#include "fun42dm.h"
#include "ui_fun42dm.h"
#include "../tooltextinputdialog.h"

/**
 * 打磨数:针对派工单的每个件都有自己的.
 */
Fun42dm::Fun42dm(ButtonStatus *btStatus, QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::Fun42dm)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	ui->tableWidgetValue->setSelectionBehavior(QAbstractItemView::SelectRows); //单击选择一行
	ui->tableWidgetValue->setSelectionMode(QAbstractItemView::SingleSelection); //设置只能选择一行，不能多行选中
	ui->tableWidgetValue->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置每行内容不能编辑
	ui->tableWidgetValue->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); //去掉水平滚动条
	ui->tableWidgetValue->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui->tableWidgetValue->horizontalHeader()->setStretchLastSection(true);
	this->btStatus = btStatus;
	ShowOrderInfo();
}

Fun42dm::~Fun42dm()
{
	delete ui;
}

void Fun42dm::on_btSave_clicked()
{
	QList<int> valueList;
	{
		int sum = 0;
		for (int i = 0; i < m_OrderIndex.BoySum; i++)
		{
			QTableWidgetItem *item = ui->tableWidgetValue->item(i, 1);
			if (item == 0)
				return;
			valueList.append(item->text().toInt());
			sum += item->text().toInt();
		}
		if (sum == 0)
		{
			MESMessageDlg::about(0, "提示", "未录入数据");
			return;
		}
	}

	Order &m_order = OrderMainOperation::GetInstance().mainOrderCache;

	QualityRegulate info;
	info.setDispatchNo(m_order.getMoDispatchNo()); //	20	ASC	派工单号
	info.setDispatchPrior(m_order.getMoDispatchPrior()); // 30	ASC	派工项次
	info.setProcCode(m_order.getMoProcCode()); //	20	ASC	工序代码
	info.setStaCode(m_order.getMoStaCode()); //10	ASC	站别代码
	info.setStartCardNo(btStatus->getBindText()); //	10	ASC	开始卡号
	info.setStartTime(btStatus->getChanageTime()); //	6	HEX	开始时间
	info.setEndCardNo(btStatus->getBindText()); //	10	ASC	结束卡号
	info.setEndTime(Tool::GetCurrentDateTimeStr()); //	6	HEX	结束时间
	info.setCardType(2); //	1	HEX	刷卡原因编号:巡机、打磨等 1代表巡机  2代表打磨
	for (int i = 0; i < m_order.orderBoyList.size(); i++)
	{
		//20	ASC	产品编号
		//	4	DWORD	本次记录总数
		QualityRegulate::FuncData data;
		data.ItemNO = m_order.orderBoyList[i].getPcsItemNo();
		data.CurChangeQty = valueList[i];
		info.getDataList().append(data);
		//
		m_order.orderBoyList[i].OnAddCurClassPolishNo(valueList[i]);
	}
	//再保存到数据库
	Notebook("打磨", info).insert();
	ShowOrderInfo();

	ToastDialog::Toast(NULL,"保存成功",1000);
}

void Fun42dm::on_btExit_clicked()
{
	//须要刷卡
	close();
}

void Fun42dm::ShowOrderInfo()
{
	Order &order = OrderMainOperation::GetInstance().mainOrderCache;

	m_OrderIndex.BoySum = order.orderBoyList.size();

	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;
	OrderMainOperation::OrderToQMap(order, orderMap, m_OrderIndex.BoyIndex,
			orderBoyMap);
	orderMap.unite(orderBoyMap);
	for (int i = 0; i < ui->tableWidgetInfo->rowCount(); i++)
	{
		QTableWidgetItem *item = ui->tableWidgetInfo->item(i, 0);
		if (item != NULL)
		{
			QString name = item->text();
			QString value = orderMap[name];
			ui->tableWidgetInfo->setItem(i, 1, new QTableWidgetItem(value));
		}
	}
	//
	ui->tableWidgetValue->setRowCount(m_OrderIndex.BoySum);

	for (int i = 0; i < m_OrderIndex.BoySum; i++)
	{
		ui->tableWidgetValue->setItem(i, 0,
				new QTableWidgetItem(order.orderBoyList[i].getPcsItemNo()));
		ui->tableWidgetValue->setItem(i, 1, new QTableWidgetItem("0"));
	}
}

void Fun42dm::on_tableWidgetValue_itemClicked(QTableWidgetItem *item)
{
	ToolTextInputDialog dialog(0, ToolTextInputDialog::typeNumber, "", "请录入打磨数量");
	if (dialog.exec() == ToolTextInputDialog::KeyHide)
		return;

	ui->tableWidgetValue->setItem(item->row(), 1,
			new QTableWidgetItem(dialog.text()));
}
