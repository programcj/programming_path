#include "fun24fsgz.h"
#include "ui_fun24fsgz.h"
#include <QMessageBox>
#include "../toolicscandialog.h"
#include "../plugin/mesmessagedlg.h"

/**
 * 显示列表左则:
 * 机器ID:
 * 设备ID:
 * 故障类型:
 * 未处理记录:
 * 24	辅设故障
 * 25	机器故障
 * 26	模具故障
 */
Fun24fsgz::Fun24fsgz(ButtonStatus *bt, QWidget *parent) :
        QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::Fun24fsgz)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	this->btStatus = bt;
	if (bt->funIndex == 24)
	{
		ui->groupBox_3->setShown(false);
	}
	loading();
}

Fun24fsgz::~Fun24fsgz()
{
	delete ui;
}

void Fun24fsgz::on_btExit_clicked()
{
	close();
}

//故障开始
void Fun24fsgz::on_btStart_clicked()
{
	//改变按钮为开始刷卡 再产生刷卡记录
	if (btStatus->isShowStart())
	{
		MESMessageDlg::about(0, "提示", "己开始");
		return;
	}

	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
    saveIC(ic.getICCardNo());

	//刷卡数据携带长度为0
	BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache, ic.getICCardNo(), btStatus->getKeyInfo()->funIndex,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook(btStatus->name + "开始刷卡", brush).insert();
	btStatus->setShowStartCard(); //显示为开始卡
}
//故障结束
void Fun24fsgz::on_btEnd_clicked()
{
	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(0, "提示", "己结束");
		return;
	}
	{ //判断是否还有没有完成的故障
		for (int i = 0; i < m_list.size(); i++)
		{
			if (m_list[i].status != "完成")
			{
				MESMessageDlg::about(0, "提示", "还有未完成,不可结束");
				return;
			}
		}
	}

	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
	btStatus->setBindText(ic.getICCardNo());
	BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache, ic.getICCardNo(), btStatus->getKeyInfo()->funIndex,
			Tool::GetCurrentDateTimeStr(), 1);
	Notebook(btStatus->name + "结束刷卡", brush).insert();
	btStatus->setShowEndCard();
    saveIC("");
}

//加载
void Fun24fsgz::loading()
{
	MESFaultTypeCfg::TypeInfo *info = AppInfo::fault_type_cfg.getFault(
			btStatus->name);
	this->setWindowTitle(btStatus->name);

    logInfo("帮定内容:"+btStatus->getBindText());

	if (info != NULL)
	{
		//增加从文件中读取,和保存功能
		{
			QFile file(AppInfo::getPath_Tmp() + btStatus->name + ".txt");

			/* 如果文件不存在时，就从配置中读取配置
			 * 如果存在，就从 文件中读取
			 */
			if (!QFile::exists(
					AppInfo::getPath_Tmp() + btStatus->name + ".txt"))
			{

				QDomDocument doc;

				for (int i = 0; i < info->getTypeList().size(); i++)
				{
					FaultItem item;
					item.index = info->getTypeList()[i].Index;
					item.name = info->getTypeList()[i].Type;
					item.status = "完成";
					m_list.append(item);
				}

				PropertyListToXML(doc, m_list);
				if (file.open(QIODevice::WriteOnly | QIODevice::Text))
				{
					QTextStream out(&file);
					out << doc.toString();
					file.close();
				}
			}
			else
			{
				/*
				 * 加载本地按钮状态 与 配置文件中的按钮绑定 到 uiBtStatus
				 * 当然. 这里存在信息丢失问题
				 *    第一次配置 12345 刷卡5开始
				 *      配置更新  1234  没有5
				 *      配置更新 12345 这里就没有5
				 */
				QList<FaultItem> m_tmp; //UI按钮状态
				if (file.open(QIODevice::ReadOnly | QFile::Text))
				{
					XMLFileToPropertList(file, m_tmp);
					file.close();
				}

				int size = info->getTypeList().size();
				m_list.clear();
				for (int i = 0; i < size; i++)
				{
					FaultItem item;
					item.index = info->getTypeList()[i].Index;
					item.name = info->getTypeList()[i].Type;
					item.status = "完成";

					int tmpSize = m_tmp.size();
					for (int j = 0; j < tmpSize; j++)
					{
						if (m_tmp[j].index == item.index)
						{
							//m_tmp[j].name = item.name;
							item.status = m_tmp[j].status;
						}
					}
					m_list.append(item);
				}
			}
		}
		//////////////////
		ui->tableWidget_2->setColumnCount(2);
		ui->tableWidget_2->setRowCount(m_list.size());

		for (int i = 0; i < m_list.size(); i++)
		{
			ui->tableWidget_2->setItem(i, 0,
					new QTableWidgetItem(m_list[i].name));
			ui->tableWidget_2->setItem(i, 1,
					new QTableWidgetItem(m_list[i].status));
		}

		ui->tableWidget_2->horizontalHeader()->setResizeMode(0,
				QHeaderView::ResizeToContents);
	}
	show();
}
//显示
void Fun24fsgz::show()
{
	QString str;
	ui->tableWidget->setItem(0, 1,
			new QTableWidgetItem(AppInfo::GetInstance().getDevId()));
	ui->tableWidget->setItem(1, 1,
			new QTableWidgetItem(AppInfo::GetInstance().getMachineId()));
	ui->tableWidget->setItem(2, 1, new QTableWidgetItem(this->windowTitle()));

	int noProcessCount = 0;
	for (int i = 0; i < m_list.size(); i++)
	{
		if (m_list[i].status != "完成")
		{
			noProcessCount++;
		}
	}
	str.sprintf("%d", noProcessCount);

	ui->tableWidget->setItem(3, 1, new QTableWidgetItem(str));
    ui->lineEditText->setText(Tool::IntToQString(getTmpTime()));
}

//设备报障 或  NG OK
void Fun24fsgz::on_tableWidget_2_itemClicked(QTableWidgetItem *item)
{
	MESFaultTypeCfg::TypeInfo *info = AppInfo::fault_type_cfg.getFault(
			btStatus->name);
	if (info == NULL)
		return;
	FaultItem faultitem;
	faultitem.index = info->getTypeList()[item->row()].Index;
	faultitem.name = info->getTypeList()[item->row()].Type;

	int FaultNo = faultitem.index; //这里的故障序号请使用配置文件中的序号
	FaultItem &vItem = m_list[item->row()];

	MESMessageDlg msgBox;

	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(0, "提示", "故障未开始");
		return;
	}

	if (vItem.status == "完成")
	{
        msgBox.setText("是否 设备报障?");
        QPushButton *Button1 = msgBox.addButton("设备报障",
				QDialogButtonBox::NoRole);
		msgBox.addButton("取消", QDialogButtonBox::RejectRole);
		msgBox.exec();
		if (msgBox.clickedButton() == Button1)
        { //设备报障
			vItem.status = "待维修确认";

			//这里刷开始卡,还有待维修时间哦
			BrushCard::CardFault fault;
			fault.FaultNo = FaultNo; //	1	HEX	故障序号
			fault.ResultNo = 0; //	1	HEX	预计修理时间          

            BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache,
                            getIC(),
					btStatus->getKeyInfo()->funIndex,
					Tool::GetCurrentDateTimeStr(), 0);
			brush.setCarryDataValut(fault);
			Notebook(btStatus->name + "待维修确认", brush).insert();

			saveTmpFaultNo(FaultNo);
		}
	}
	else
	{ //选择NG OK
		QPushButton *Button1 = msgBox.addButton("OK",
				QDialogButtonBox::YesRole);
		QPushButton *Button2 = msgBox.addButton("NG", QDialogButtonBox::NoRole);
		msgBox.addButton("取消", QDialogButtonBox::RejectRole);
		msgBox.setText("维修确认选择?");
		msgBox.exec();
		if (msgBox.clickedButton() == Button1)
		{ //OK
			BrushCard::CardFault fault;
			fault.FaultNo = FaultNo; //	1	HEX	故障序号
			fault.ResultNo = 1; //	1	HEX	0 NG 1 OK

            BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache, getIC(),
					btStatus->getKeyInfo()->funIndex,
					Tool::GetCurrentDateTimeStr(), 1); //这里刷结束卡
			brush.setCarryDataValut(fault);
			Notebook(btStatus->name + "维修确认", brush).insert();
			vItem.status = "完成";
		}
		if (msgBox.clickedButton() == Button2)
		{ //NG
			BrushCard::CardFault fault;
			fault.FaultNo = FaultNo; //	1	HEX	故障序号
			fault.ResultNo = 0; //	1	HEX	0 NG 1 OK
            BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache, getIC(),
					btStatus->getKeyInfo()->funIndex,
					Tool::GetCurrentDateTimeStr(), 1); //这里刷结束卡
			brush.setCarryDataValut(fault);
			vItem.status = "完成";
			Notebook(btStatus->name + "维修确认", brush).insert();
		}
	}

	//保存
	QFile file(AppInfo::getPath_Tmp() + btStatus->name + ".txt");
	QDomDocument doc;
	PropertyListToXML(doc, m_list);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << doc.toString();
		file.close();
	}
	ui->tableWidget_2->item(item->row(), 1)->setText(vItem.status);
	show();
}

//设置预计维修时间
void Fun24fsgz::on_btSaveTime_clicked()
{
	//这里刷开始卡,还有待维修时间哦
	int v = ui->lineEditText->text().toInt();
	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(0, "提示", "故障未开始");
		return;
	}
	if (v == 0)
	{
		ToastDialog::Toast(NULL, "预计时间为0,不可保存", 2000);
		return;
	}
	v = v * 100;
////
	BrushCard::CardFault fault;
	fault.FaultNo = getTmpFaultNo(); //	1	HEX	故障序号 最后一次的故障序号
	fault.ResultNo = v; //	1	HEX	预计修理时间 上传须乘100

    BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache, getIC(),
			btStatus->getKeyInfo()->funIndex, Tool::GetCurrentDateTimeStr(), 0);
	brush.setCarryDataValut(fault);
	Notebook(btStatus->name + "待维修确认", brush).insert();
	ToastDialog::Toast(NULL, "保存成功", 1000);
	saveTmpTime(v / 100);
}

//预计修理时间 0卡号 1预计时间 2最后的故障序号
void Fun24fsgz::saveTmpTime(int v)
{
	QString str = btStatus->getBindText();
	QStringList list = str.split("|");

    QString ic;
    QString time;
    QString no;

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];

    time.sprintf("%d", v);

    btStatus->setBindText(ic+"|"+time+"|"+no);
}
//故障序号 最后一次的故障序号
void Fun24fsgz::saveTmpFaultNo(int number)
{
    QString str = btStatus->getBindText();
    QStringList list = str.split("|");

    QString ic;
    QString time;
    QString no;

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];

    no.sprintf("%d", number);

    btStatus->setBindText(ic+"|"+time+"|"+no);
}
//故障序号 最后一次的故障序号
int Fun24fsgz::getTmpFaultNo()
{
    QString str = btStatus->getBindText();
    QStringList list = str.split("|");

    QString ic;
    QString time;
    QString no="0";

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];
    return no.toInt();
}

//预计修理时间
int Fun24fsgz::getTmpTime()
{
    QString str = btStatus->getBindText();
    QStringList list = str.split("|");

    QString ic;
    QString time;
    QString no="0";

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];
    return time.toInt();
}
//保存卡号
void Fun24fsgz::saveIC(const QString &newIC)
{
    QString str = btStatus->getBindText();
    QStringList list = str.split("|");

    QString ic;
    QString time;
    QString no;

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];

    ic=newIC;
    btStatus->setBindText(ic+"|"+time+"|"+no);
}

//获取卡号
QString Fun24fsgz::getIC()
{
    QString str = btStatus->getBindText();
    QStringList list = str.split("|");

    QString ic="00000001";
    QString time;
    QString no;

    if (list.size() > 0)
        ic = list[0];
    if (list.size() > 1)
        time = list[1];
    if(list.size() > 2)
        no=list[1];

    return ic;
}

//加1
void Fun24fsgz::on_btPlus1_clicked()
{   
    if (btStatus->isShowEnd())
    {
        MESMessageDlg::about(0, "提示", "故障未开始");
        return;
    }

    int v=ui->lineEditText->text().toInt();
    v++;
    ui->lineEditText->setText(Tool::IntToQString(v));
}

//减1
void Fun24fsgz::on_btSub1_clicked()
{
    if (btStatus->isShowEnd())
    {
        MESMessageDlg::about(0, "提示", "故障未开始");
        return;
    }

    int v=ui->lineEditText->text().toInt();
    if(v==0)
        return;
    v--;
    ui->lineEditText->setText(Tool::IntToQString(v));
}
