#include "fun43ordermove.h"
#include "../plugin/mesmessagedlg.h"

/**
 * 工单调拨
 *
 */
Fun43OrderMove::Fun43OrderMove(ButtonStatus *btStatus, QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint)
{
	ui.setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

    this->btStatus = btStatus;
	loading();
}

Fun43OrderMove::~Fun43OrderMove()
{

}
//上一单 参考 生产工单 界面
void Fun43OrderMove::on_btOrderUp_clicked()
{
	if (m_orderIndex.Index > 1)
	{
		m_orderIndex.Index--;
		m_orderIndex.BoyIndex = 0;
	}
	showOrderTableInfo();
}

//下一单 参考 生产工单 界面
void Fun43OrderMove::on_btOrderNext_clicked()
{
	if (m_orderIndex.Index < Order::count())
	{
		m_orderIndex.Index++;
		m_orderIndex.BoyIndex = 0;
	}
	showOrderTableInfo();
}

//退出
void Fun43OrderMove::on_btExit_clicked()
{
	close();
}

//确认调拨
//1 防止给自己调拨
//2 未正确选择目标机台
//3 选择目标机台吨位超出范围,目标吨位在本机 最大 最小 之间
//4 良品数己超过生产总数，不能调拨
void Fun43OrderMove::on_btOK_clicked()
{
    QList<QTableWidgetItem*> list = ui.tableWidget_2->selectedItems();
	if (list.size() == 0)
		return;

	BrushCard::CardMachineNo cardMachineNo; //新机器编号
	cardMachineNo.MachineNo =
			AppInfo::GetInstance().mac_ton_cfg.TonList[list[0]->row()].getMachine();
	if (cardMachineNo.MachineNo == myMachineNo)
	{
		ToastDialog::Toast(NULL, "不能给自己调拨",1500);
		return;
	}
	if (cardMachineNo.MachineNo.length() == 0)
	{
		ToastDialog::Toast(NULL, "未选择机台",1500);
		return;
	}
	Order order;
	if (!Order::query(order, m_orderIndex.Index))
	{
		ToastDialog::Toast(NULL, "未找到工单",1500);
		return;
	}
	if (order.getMoDispatchNo().length() == 0)
	{
		ToastDialog::Toast(NULL, "未找到工单",1500);
		return;
	}
	order.getBoy();
	int ton =
			AppInfo::GetInstance().mac_ton_cfg.TonList[list[0]->row()].getTon();
	if (ton > order.orderBoyList[0].getPcsFitMachineMax())
	{
		MESMessageDlg::about(this, "提示", "选择目标机台吨位超出范围，请重新选择！");
		return;
	}
	//良品数己超过生产总数，不能调拨，请重新选择!
	if (order.orderBoyList[0].getTotalOkNum()
			> order.orderBoyList[0].getPcsDispatchQty())
	{
		MESMessageDlg::about(this, "提示", "良品数己超过生产总数，不能调拨，请重新选择！");
		return;
	}

	//刷卡
	ToolICScanDialog ic(AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(43));
	if (ic.exec() == QDialog::Rejected)
		return;

	//43  工单调拔
	BrushCard brush(order, ic.getICCardNo(), BrushCard::AsORDER_ALLOT,
			Tool::GetCurrentDateTimeStr(), 0);
	brush.setCarryDataValut(cardMachineNo);

	Notebook("工单调拔", brush).insert();

	ToastDialog::Toast(NULL, "保存成功", 2000);
	{
		//从数据库中去掉这个工单
		order.remove();
		Order::sort(); //排序
		//如果是主单呢?则更新主单操作类
		if (order.getMainOrderFlag() == 1)
			OrderMainOperation::GetInstance().OnUpdate();
	}
	showOrderTableInfo();
}

bool Fun43OrderMove::loading()
{
	QList<MESMACTonCfg::TonInfo> &list =
			AppInfo::GetInstance().mac_ton_cfg.TonList;

	ui.tableWidget_2->setRowCount(list.size());

	for (int i = 0; i < list.size(); i++)
	{
		ui.tableWidget_2->setItem(i, 0,
				new QTableWidgetItem(list[i].getMachine()));
		ui.tableWidget_2->setItem(i, 1,
				new QTableWidgetItem(Tool::IntToQString(list[i].getTon())));

		if (list[i].getIpaddress() == AppInfo::GetInstance().getDevIp())
			myMachineNo = list[i].getMachine(); //自己的机器编号
	}
	if (myMachineNo.length() == 0)
	{
		//MESMessageDlg::about(NULL,"提示","没有自己的机器编号");
		//return false;
	}
	showOrderTableInfo();
	return true;
}

//显示工单信息
void Fun43OrderMove::showOrderTableInfo()
{
	Order order;
	if (Order::query(order, m_orderIndex.Index))
		order.getBoy();
	m_orderIndex.BoySum = order.orderBoyList.size();
	//
	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;

	OrderMainOperation::OrderToQMap(order, orderMap, m_orderIndex.BoyIndex,
			orderBoyMap);

	orderMap.insert("机器编号", myMachineNo);
	orderMap.unite(orderBoyMap);

	if (m_orderIndex.Index == 1)
		orderMap.insert("工单类型", "主单");
	else
		orderMap.insert("工单类型", "次单");

	for (int i = 0; i < ui.tableWidget->rowCount(); i++)
	{
		QString key = ui.tableWidget->item(i, 0)->text();
		QString v = orderMap[key];
		if (ui.tableWidget->item(i, 1) == NULL)
			ui.tableWidget->setItem(i, 1, new QTableWidgetItem(v));
		else
			ui.tableWidget->item(i, 1)->setText(v);
	}
}

void Fun43OrderMove::on_tableWidget_2_itemClicked(QTableWidgetItem *item)
{
	ui.lineEdit->setText(ui.tableWidget_2->item(item->row(), 0)->text());
}
