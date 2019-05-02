#include "fun45sbdj.h"
#include "ui_fun45sbdj.h"
#include <QNetworkInterface>
#include "../plugin/mesmessagedlg.h"
//设备点检
Fun45sbdj::Fun45sbdj(QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::Fun45sbdj)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
}

Fun45sbdj::~Fun45sbdj()
{
	delete ui;
}

void Fun45sbdj::on_btSave_clicked()
{
	MES45Table *table = NULL;
	if (ui->comboBox->currentIndex() == 0)
	{ //机器
		table = &Tab1;
	}
	if (ui->comboBox->currentIndex() == 1)
	{ //模具
		table = &Tab2;
	}
	if (table == NULL)
		return;

	ToolICScanDialog ic(AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(45));
	if (ic.exec() == QDialog::Rejected)
		return;

	OtherSetInfo info;
	OtherSetInfo::DevInspection devInspenct;

	devInspenct.DispatchNo =
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo();	//	20	ASC	派工单号
	devInspenct.DispatchPrior =
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchPrior();//	30	ASC	派工项次
	devInspenct.ProcCode =
			OrderMainOperation::GetInstance().mainOrderCache.getMoProcCode();//	20	ASC	工序代码
	devInspenct.StaCode =
			OrderMainOperation::GetInstance().mainOrderCache.getMoStaCode();//	10	ASC	站别代码
	devInspenct.CardID = ic.getICCardNo();	//	10	HEX	卡号
	devInspenct.CardDate = Tool::GetCurrentDateTimeStr();	//	6	HEX	点检时间

	for (int i = 0; i < table->List.size(); i++)
	{
		OtherSetInfo::InspectionProj proj;
		proj.NO = table->List[i].number;
		proj.Result = table->List[i].value;
		proj.Brand = MachineBrand;
		devInspenct.nDataList.append(proj);
	}

	info.setDevInspection(devInspenct);
	Notebook("设备点检", info).insert();
}
//未开机
void Fun45sbdj::on_btNotStart_clicked()
{
	MES45Table *table = NULL;
	if (ui->comboBox->currentIndex() == 0)
	{ //机器
		table = &Tab1;
	}
	if (ui->comboBox->currentIndex() == 1)
	{ //模具
		table = &Tab2;
	}
	if (table == NULL)
		return;

	ToolICScanDialog ic(AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(45));
	if (ic.exec() == QDialog::Rejected)
		return;

	OtherSetInfo info;
	OtherSetInfo::DevInspection devInspenct;

	devInspenct.DispatchNo =
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo();	//	20	ASC	派工单号
	devInspenct.DispatchPrior =
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchPrior();//	30	ASC	派工项次
	devInspenct.ProcCode =
			OrderMainOperation::GetInstance().mainOrderCache.getMoProcCode();//	20	ASC	工序代码
	devInspenct.StaCode =
			OrderMainOperation::GetInstance().mainOrderCache.getMoStaCode();//	10	ASC	站别代码
	devInspenct.CardID = ic.getICCardNo();	//	10	HEX	卡号
	devInspenct.CardDate = Tool::GetCurrentDateTimeStr();	//	6	HEX	点检时间

	for (int i = 0; i < table->List.size(); i++)
	{
		OtherSetInfo::InspectionProj proj;
		proj.NO = table->List[i].number;
		proj.Result = 2;
		proj.Brand = MachineBrand;
		devInspenct.nDataList.append(proj);
	}

	info.setDevInspection(devInspenct);
	Notebook("设备点检", info).insert();
	ToastDialog::Toast(NULL, "保存成功", 2000);
}

void Fun45sbdj::on_btExit_clicked()
{
	close();
}

void Fun45sbdj::on_comboBox_currentIndexChanged(int index)
{
	ShowTableInfo();
}

bool Fun45sbdj::Loading()
{
	//获取本机品牌
	MachineBrand = "";

	//QList<QHostAddress> ipList = QNetworkInterface::allAddresses();
	QString ip =AppInfo::GetInstance().getDevIp();
	if (ip.length() == 0)
	{
		//没有 这台机器的IP
		ToastDialog::Toast(NULL, "没有 这台机器的IP", 2000);
		return false;
	}

	for (int i = 0; i < AppInfo::mac_ton_cfg.TonList.size(); i++)
	{
		if (AppInfo::mac_ton_cfg.TonList[i].getIpaddress() == ip)
		{
			MachineBrand = AppInfo::mac_ton_cfg.TonList[i].getMachinoBrand();
			break;
		}
	}
	if (MachineBrand.length() == 0)
	{
		//没有 这台机器的品牌
		MESMessageDlg::about(NULL, "提示", "没有这台机器的品牌");
		return false;
	}
	ui->lineEdit->setText(MachineBrand);
	int size = AppInfo::check_item_cfg.List.size();
	for (int i = 0; i < size; i++)
	{
		logDebug(AppInfo::check_item_cfg.List[i].ModelName);

		if (AppInfo::check_item_cfg.List[i].ModelName == MachineBrand)
		{
			MESCheckItemCfg::CheckModel &model = AppInfo::check_item_cfg.List[i];

			Tab1.List.clear();
			Tab2.List.clear();

			for (int i = 0; i < model.MachineList.size(); i++)
			{
				Tab1.insert(model.MachineList[i].getIndex(),
						model.MachineList[i].getName());
			}

			for (int i = 0; i < model.ModeList.size(); i++)
			{
				Tab2.insert(model.ModeList[i].getIndex(),
						model.ModeList[i].getName());
			}
		}
	}
	return true;
}

void Fun45sbdj::showEvent(QShowEvent* e)
{
	ShowTableInfo();
}

void Fun45sbdj::ShowTableInfo()
{
	ui->tableWidget->clear();
	MES45Table *table = NULL;
	if (ui->comboBox->currentIndex() == 0)
	{ //机器
		table = &Tab1;
	}
	if (ui->comboBox->currentIndex() == 1)
	{ //模具
		table = &Tab2;
	}
	if (table == NULL)
		return;

	ui->tableWidget->setRowCount(table->List.size());

	for (int i = 0; i < table->List.size(); i++)
	{
		ui->tableWidget->setItem(i, 0,
				new QTableWidgetItem(table->List[i].name));

		if (table->List[i].value == 0) // 0：NG 1:OK  2:未开机
			ui->tableWidget->setItem(i, 1, new QTableWidgetItem("NG"));

		if (table->List[i].value == 1) // 0：NG 1:OK  2:未开机
			ui->tableWidget->setItem(i, 1, new QTableWidgetItem("OK"));

		if (table->List[i].value == 2) // 0：NG 1:OK  2:未开机
			ui->tableWidget->setItem(i, 1, new QTableWidgetItem("未开机"));
	}
}

void Fun45sbdj::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
	MES45Table *table = NULL;
	if (ui->comboBox->currentIndex() == 0)
	{ //机器
		table = &Tab1;
	}
	if (ui->comboBox->currentIndex() == 1)
	{ //模具
		table = &Tab2;
	}
	if (table == NULL)
		return;

	MESMessageDlg msgBox;
	msgBox.setText("选择 NG 或 OK?");
	QPushButton *Button1 = msgBox.addButton("OK", QDialogButtonBox::YesRole);
	QPushButton *Button2 = msgBox.addButton("NG", QDialogButtonBox::NoRole);
	msgBox.addButton("取消", QDialogButtonBox::RejectRole);
	msgBox.exec();
	if (msgBox.clickedButton() == Button1)
	{ //OK
		table->List[item->row()].value = 1;
	}
	if (msgBox.clickedButton() == Button2)
	{ //NG
		table->List[item->row()].value = 0;
	}
	ShowTableInfo();
}
