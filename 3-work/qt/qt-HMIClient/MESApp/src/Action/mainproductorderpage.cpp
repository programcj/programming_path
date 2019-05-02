#include "mainproductorderpage.h"
#include "ui_mainproductorderpage.h"
#include "tooltextinputdialog.h"
#include "toolicscandialog.h"
#include <QTableWidgetItem>
#include <QMessageBox>
#include "plugin/toastdialog.h"
#include "fun_dialog/syncorderdialog.h"

MainProductOrderPage::MainProductOrderPage(QWidget *parent) :
		QWidget(parent), ui(new Ui::MainProductOrderPage)
{
	ui->setupUi(this);

	showOrder();

	uiPageButton.appendButton(ui->pushButton_1);
    uiPageButton.appendButton(ui->pushButton_2);
	uiPageButton.appendButton(ui->pushButton_3);
	uiPageButton.appendButton(ui->pushButton_4);

	uiPageButton.appendButton(ui->pushButton_5);
	uiPageButton.appendButton(ui->pushButton_8);
	uiPageButton.appendButton(ui->pushButton_9);
	uiPageButton.appendButton(ui->pushButton_10);

	uiPageButton.appendButton(ui->pushButton_11);
	uiPageButton.appendButton(ui->pushButton_12);
	uiPageButton.appendButton(ui->pushButton_13);
	uiPageButton.appendButton(ui->pushButton_14);

	uiPageButton.appendButton(ui->pushButton_15);
	uiPageButton.appendButton(ui->pushButton_16);
	uiPageButton.appendButton(ui->pushButton_17);
	uiPageButton.appendButton(ui->pushButton_18);

    uiPageButton.appendButton(ui->pushButton_19);
    uiPageButton.appendButton(ui->pushButton_20);
    uiPageButton.appendButton(ui->pushButton_21);
    uiPageButton.appendButton(ui->pushButton_22);
	uiPageButton.init();

	connect(&uiPageButton, SIGNAL(funButton21(int)), this,
			SLOT(funButton21(int))); /*换模      */
	connect(&uiPageButton, SIGNAL(funButton22(int)), this,
			SLOT(funButton22(int))); /*换料      */
	connect(&uiPageButton, SIGNAL(funButton23(int)), this,
			SLOT(funButton23(int))); /*换单      */
	connect(&uiPageButton, SIGNAL(funButton24(int)), this,
			SLOT(funButton24(int))); /*辅设故障  */
	connect(&uiPageButton, SIGNAL(funButton25(int)), this,
			SLOT(funButton25(int))); /*机器故障  */
	connect(&uiPageButton, SIGNAL(funButton26(int)), this,
			SLOT(funButton26(int))); /*模具故障  */
	connect(&uiPageButton, SIGNAL(funButton27(int)), this,
			SLOT(funButton27(int))); /*待料      */
	connect(&uiPageButton, SIGNAL(funButton28(int)), this,
			SLOT(funButton28(int))); /*保养      */
	connect(&uiPageButton, SIGNAL(funButton29(int)), this,
			SLOT(funButton29(int))); /*待人      */
	connect(&uiPageButton, SIGNAL(funButton30(int)), this,
			SLOT(funButton30(int))); /*交接班刷卡*/
	connect(&uiPageButton, SIGNAL(funButton31(int)), this,
			SLOT(funButton31(int))); /*原材料不良*/
	connect(&uiPageButton, SIGNAL(funButton32(int)), this,
			SLOT(funButton32(int))); /*计划停机  */
	connect(&uiPageButton, SIGNAL(funButton33(int)), this,
			SLOT(funButton33(int))); /*上班      */
	connect(&uiPageButton, SIGNAL(funButton34(int)), this,
			SLOT(funButton34(int))); /*下班      */
	connect(&uiPageButton, SIGNAL(funButton35(int)), this,
			SLOT(funButton35(int))); /*修改周期  */
	connect(&uiPageButton, SIGNAL(funButton36(int)), this,
			SLOT(funButton36(int))); /*调机      */
	connect(&uiPageButton, SIGNAL(funButton37(int)), this,
			SLOT(funButton37(int))); /*调单      */
	connect(&uiPageButton, SIGNAL(funButton38(int)), this,
			SLOT(funButton38(int))); /*调整模穴  */
	connect(&uiPageButton, SIGNAL(funButton39(int)), this,
			SLOT(funButton39(int))); /*工程等待  */
	connect(&uiPageButton, SIGNAL(funButton40(int)), this,
			SLOT(funButton40(int))); /*投料      */
	connect(&uiPageButton, SIGNAL(funButton41(int)), this,
			SLOT(funButton41(int))); /*巡机      */
	connect(&uiPageButton, SIGNAL(funButton42(int)), this,
			SLOT(funButton42(int))); /*打磨      */
	connect(&uiPageButton, SIGNAL(funButton43(int)), this,
			SLOT(funButton43(int))); /*工单调拨  */
	connect(&uiPageButton, SIGNAL(funButton44(int)), this,
			SLOT(funButton44(int))); /*试模      */
	connect(&uiPageButton, SIGNAL(funButton45(int)), this,
			SLOT(funButton45(int))); /*设备点检  */
	connect(&uiPageButton, SIGNAL(funButton46(int)), this,
			SLOT(funButton46(int))); /*耗电量    */
	connect(&uiPageButton, SIGNAL(funButton47(int)), this,
			SLOT(funButton47(int))); /*试料      */

	connect(&CSCPAction::GetInstance(), SIGNAL(OnSignalOrderTableChange()),
			this, SLOT(OnOrderTableChange()));

	connect(&OrderMainOperation::GetInstance(),
			SIGNAL(OnSignalUpdateMainOrder()), this,
			SLOT(OnOrderTableChange()));

	connect(&CSCPAction::GetInstance(),
			SIGNAL(OnSignalConfigRefurbish(const QString &)), this,
			SLOT(OnConfigRefurbish(const QString &)));

}

MainProductOrderPage::~MainProductOrderPage()
{
	delete ui;
}

//显示工单信息
void MainProductOrderPage::showOrder()
{
	Order order;
	if (Order::query(order, orderIndex.Index))
		order.getBoy();
	orderIndex.BoySum = order.orderBoyList.size();
//
	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;

	OrderMainOperation::OrderToQMap(order, orderMap, orderIndex.BoyIndex,
			orderBoyMap);

	orderMap.unite(orderBoyMap);

    if (orderIndex.Index == 1)
        ui->labelOrderType->setText("主");
    else
        ui->labelOrderType->setText("次");
    if (order.getMoDispatchNo().length() == 0)
            ui->labelOrderType->setText("空");
    QString str;
    str.sprintf("共%d单 - 第%d单 第%d件/共%d件",Order::count(),order.getMainOrderFlag(),
                orderIndex.BoyIndex+1,order.getMoMultiNum());

    ui->labelOrderNo->setText("派工单号:"+orderMap["派工单号"]);
    ui->labelOrderTitle->setText(str);

    ui->tableWidgetOrder->setItem(0,1,new QTableWidgetItem(orderMap["工单号"]));
    ui->tableWidgetOrder->setItem(1,1,new QTableWidgetItem(orderMap["产品编号"]));
    ui->tableWidgetOrder->setItem(2,1,new QTableWidgetItem(orderMap["产品描述"]));
    ui->tableWidgetOrder->setItem(3,1,new QTableWidgetItem(orderMap["模具编号"]));

    ui->tableWidgetOrderBoy->setItem(0,1,new QTableWidgetItem(orderMap["生产总数"]));
    ui->tableWidgetOrderBoy->setItem(0,3,new QTableWidgetItem(orderMap["派工数量"]));
    ui->tableWidgetOrderBoy->setItem(1,1,new QTableWidgetItem(orderMap["次品总数"]));
    ui->tableWidgetOrderBoy->setItem(1,3,new QTableWidgetItem(orderMap["调机数"]));
    ui->tableWidgetOrderBoy->setItem(2,1,new QTableWidgetItem(orderMap["模具可用模穴数"]));//本件模穴数
    ui->tableWidgetOrderBoy->setItem(2,3,new QTableWidgetItem(orderMap["产品可用模穴数"]));//使用模穴数

    ui->tableWidgetClass->setItem(1,1,new QTableWidgetItem(orderMap["本班生产总数"]));
    ui->tableWidgetClass->setItem(1,2,new QTableWidgetItem(orderMap["本班次品总数"]));
    ui->tableWidgetClass->setItem(1,3,new QTableWidgetItem(orderMap["本班打磨数"]));
    ui->tableWidgetClass->setItem(1,4,new QTableWidgetItem(orderMap["本班巡机数"]));
}

//上一单
void MainProductOrderPage::on_btUpOrder_clicked()
{
	if (orderIndex.Index > 1)
	{
		orderIndex.Index--;
		orderIndex.BoyIndex = 0;
	}
	showOrder();
}
//下一单
void MainProductOrderPage::on_btNextOrder_clicked()
{
	if (orderIndex.Index < Order::count())
	{
		orderIndex.Index++;
		orderIndex.BoyIndex = 0;
	}
	showOrder();
}
//上一件
void MainProductOrderPage::on_btUpItem_clicked()
{
	if (orderIndex.BoyIndex > 0)
	{
		orderIndex.BoyIndex--;
	}
	showOrder();
}
//下一件
void MainProductOrderPage::on_btNextItem_clicked()
{
	if (orderIndex.BoyIndex < orderIndex.BoySum - 1)
	{
		orderIndex.BoyIndex++;
	}
	showOrder();
}
//上一页
void MainProductOrderPage::on_btUpPage_clicked()
{
	uiPageButton.upPage();
}

//下一页
void MainProductOrderPage::on_btNextPage_clicked()
{
	uiPageButton.downPage();
}

/**********************************************************************
 *   1. 依据多停机卡  判断是否有其它停机卡
 *   2. 按下前 显示为结束状态 改变为 开始状态 setShowStartCard
 *   3. 按下前 显示为开始状态 改变为 结束状态 setShowEndCard
 **********************************************************************/
bool MainProductOrderPage::OtherStopCardCheck(ButtonStatus *btStatus)
{
//是否允许多种停机状态	0
	if (!AppInfo::GetInstance().sys_func_cfg.isMultipleStopStatus())
	{
		if (!btStatus->isShowStart()) //如果是显示为结束状态,表示 现在要开始操作这个功能了 判断是否还有其它停机卡
		{
			if (uiPageButton.getAllButtonStatus().isOtherStopCard(btStatus))
			{
                MESMessageDlg::about(this,"提示", "还有其它停机卡");
				return true;
			}
		}
	}
	return false;
}
//-------------------------------------------------------------------------
//	换模
//  有停机功能      就要刷两次卡
//  没有停机功能  有的刷两次卡,有的刷一次卡.
void MainProductOrderPage::funButton21(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;

	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		Order order;
		Order::query(order, 1);
		BrushCard brush(order, ic.getICCardNo(), BrushCard::AsCHANGE_MOLD,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		Order order;
		Order::query(order, 1);
		BrushCard brush(order, ic.getICCardNo(), BrushCard::AsCHANGE_MOLD,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}

//	换料 ,两次停机卡
void MainProductOrderPage::funButton22(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;

	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}

//	换单 开始,结束卡  + 更换主单
//	换单开始时不册除，换单结束时才删除主单
void MainProductOrderPage::funButton23(int funIndex)
{
//获取按钮状态
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);

	if (OtherStopCardCheck(btStatus))
		return;
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	/**
	 * 换单上传 都是上传的次单的信息头
	 */
	Order order;
	Order::query(order, 2);
	/**********************************************/
	BrushCard brush(order, ic.getICCardNo(), BrushCard::AsCHANGE_ORDER,
			Tool::GetCurrentDateTimeStr(), 0);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{	//须要删除主单
		OrderMainOperation::GetInstance().OnDelete();
		brush.setIsBeginEnd(1);
		Notebook("换单结束", brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{	//换单开始时 不用删除主单
		Notebook("换单开始", brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}
//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	辅设故障 进入子界面刷卡
void MainProductOrderPage::funButton24(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
    Fun24fsgz fun(btStatus,this);
	fun.setWindowModality(Qt::WindowModal);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	机器故障
void MainProductOrderPage::funButton25(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
    Fun24fsgz fun(btStatus,this);
    fun.setWindowModality(Qt::WindowModal);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	模具故障
void MainProductOrderPage::funButton26(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
    Fun24fsgz fun(btStatus,this);
	fun.setWindowModality(Qt::WindowModal);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	待料
void MainProductOrderPage::funButton27(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	保养
void MainProductOrderPage::funButton28(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	待人
void MainProductOrderPage::funButton29(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	交接班刷卡
void MainProductOrderPage::funButton30(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	{ //交班过程

		//求说明??????? 与班次相关的处理过程是怎么样的
		// 什么事情都不用做
	}
	Order order;
	Order::query(order, 1);
	BrushCard brush(order, ic.getICCardNo(), funIndex,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook(title, brush).insert();
}
//	原材料不良
void MainProductOrderPage::funButton31(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	计划停机
void MainProductOrderPage::funButton32(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
        if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	上班
void MainProductOrderPage::funButton33(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());

	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	BrushCard brush(order, ic.getICCardNo(), BrushCard::AsON_DUTY,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook("上班", brush).insert();
}
//	下班
void MainProductOrderPage::funButton34(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	BrushCard brush(order, ic.getICCardNo(), BrushCard::AsOFF_DUTY,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook("下班", brush).insert();
}
//	修改周期
void MainProductOrderPage::funButton35(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());

	if (ic.exec() == QDialog::Rejected)
		return;

	ToolTextInputDialog inputDialog(this, ToolTextInputDialog::typeNumber, "",
			"修改周期"); //弹出输入框输入周期
	inputDialog.setInputMask("000000;");
	if (inputDialog.exec() == ToolTextInputDialog::KeyHide)
	{
		return;
	}
	int StandCycle = inputDialog.text().toInt();
	if (StandCycle == 0)
	{
		QMessageBox::about(0, "提示", "录入数据出错");
		return;
	}
	StandCycle *= 1000; //周期*1000 单位转为ms

	//更改主单周期
	if (OrderMainOperation::GetInstance().OnUpdateStandCycle(StandCycle))
	{
		BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache,
				ic.getICCardNo(), BrushCard::AsCHANGE_STANDARD_CYCLE,
				Tool::GetCurrentDateTimeStr(), 0);
		BrushCard::CardStandCycle v(StandCycle);
		brush.setCarryDataValut(v);
		Notebook("修改周期", brush).insert();
	}
}
//	调机 有子界面, 进入时不用刷卡
void MainProductOrderPage::funButton36(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);

    Fun36tj fun(btStatus,this);
	if (fun.Loading())
		fun.exec();

	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//调单
//与服务器的工单对应 同步工单
//判断本地数据中是否还有调单数据记录。
//如果有，就不取服务器上的数据
void MainProductOrderPage::funButton37(int funIndex)
{
    if (Notebook::count("调单") == 0 && MESNet::getInstance()->stateA() == MESNet::Connected)
	{
		SyncOrderDialog dialog;
		if (dialog.exec() == QDialog::Rejected)
			return;
	}

	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	Fun37td dialog(this);//进入调单界面
	dialog.exec();
}
//	调整模穴    发送其它设置 产品模穴 , 模穴是每件产品的,所以有产品和件的区别
void MainProductOrderPage::funButton38(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

    Fun38AdjSocketNum dialog(btStatus,this);
	dialog.exec();
}
//	工程等待
void MainProductOrderPage::funButton39(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	投料 没有主单时不能投料
void MainProductOrderPage::funButton40(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());

	if (ic.exec() == QDialog::Rejected)
		return;

    Fun40tl fun(ic.getICCardNo(),this);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	巡机 每件产品的巡机
void MainProductOrderPage::funButton41(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	btStatus->setBindText(ic.getICCardNo());

    Fun41xj fun(btStatus,this);
	fun.setWindowModality(Qt::WindowModal);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	打磨 每件产品的打磨
void MainProductOrderPage::funButton42(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	btStatus->setBindText(ic.getICCardNo());

    Fun42dm fun(btStatus,this);
	fun.setWindowModality(Qt::WindowModal);
	fun.exec();

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();

}
//	工单调拨
void MainProductOrderPage::funButton43(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

    Fun43OrderMove fun(btStatus,this);
	fun.exec();
}
//	试模 开始,结束卡
void MainProductOrderPage::funButton44(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}
//	设备点检(发送其它设置)
void MainProductOrderPage::funButton45(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
	Fun45sbdj fun(this);
	if (fun.Loading())
		fun.exec();
}
//	耗电量(用电量) 手动录入
void MainProductOrderPage::funButton46(int funIndex)
{
	//手动录入
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (ic.exec() == QDialog::Rejected)
		return;
	ToolTextInputDialog inputDialog(NULL,
			ToolTextInputDialog::typeNumberDecimal, "", "请输入耗电量");
	if (inputDialog.exec() == ToolTextInputDialog::KeyHide)
		return;
	if (inputDialog.text().length() == 0)
		return;

	Order order;
	Order::query(order, 1);
	OtherSetInfo::ELEFeeCount info;
	info.DispatchNo = order.getMoDispatchNo();
	info.DispatchPrior = order.getMoDispatchPrior();
	info.ProcCode = order.getMoProcCode();
	info.CardID = ic.getICCardNo();
	info.CardDate = Tool::GetCurrentDateTimeStr();
	info.ElecNum = inputDialog.text().toInt() * 10;

	OtherSetInfo otherInfo;
	otherInfo.setELEFeeCount(info);
	Notebook("耗电量录入", otherInfo).insert();

	ToastDialog::Toast(this, "保存成功", 1000);
}
//	试料
void MainProductOrderPage::funButton47(int funIndex)
{
	ButtonStatus *btStatus = uiPageButton.getAllButtonStatus().getUIBtStatus(
			funIndex);
	ToolICScanDialog ic(btStatus->getKeyInfo());
	QString title;
	title = btStatus->name;
	if (btStatus->getKeyInfo()->isStopMachineFun())
	{ //如果是停机功能卡,就判断是否有其它停机卡
		if (OtherStopCardCheck(btStatus))
			return;
	}
	if (ic.exec() == QDialog::Rejected)
		return;
	Order order;
	Order::query(order, 1);
	if (btStatus->isShowStart()) //如果是开始卡状态 就刷结束卡
	{
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 1);
		title += "结束刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowEndCard(); //显示为结束卡
	}
	else
	{
		//如果是结束卡状态
		BrushCard brush(order, ic.getICCardNo(), funIndex,
				Tool::GetCurrentDateTimeStr(), 0);
		title += "开始刷卡";
		Notebook(title, brush).insert();
		btStatus->setShowStartCard(); //显示为开始卡
	}

	//保存按钮状态 再显示
	uiPageButton.getAllButtonStatus().XMLFileWriteUIBtStatus();
	uiPageButton.show();
}

/////////////////////////////////////////////////////////////////////////////////////////
void MainProductOrderPage::OnOrderTableChange()
{
	showOrder();
}

void MainProductOrderPage::OnConfigRefurbish(const QString& name)
{
	uiPageButton.getAllButtonStatus().XMLFileReadUIBtStatus();
	uiPageButton.init();
}
