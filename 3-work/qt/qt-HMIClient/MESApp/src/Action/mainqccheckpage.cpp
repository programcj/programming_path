#include "mainqccheckpage.h"
#include "ui_mainqccheckpage.h"
#include "tooltextinputdialog.h"
#include "plugin/toastdialog.h"

//总个数 6行 4列
#define QCLIST_ROW_MAX_COLUMN  4

MainQCCheckPage::MainQCCheckPage(QWidget *parent) :
		QWidget(parent), ui(new Ui::MainQCCheckPage)
{
	ui->setupUi(this);

	//配置文件改变信号
	connect(&CSCPAction::GetInstance(),
			SIGNAL(OnSignalConfigRefurbish(const QString &)), this,
			SLOT(OnMESConfigRefurbish(const QString &)));

	connect(&OrderMainOperation::GetInstance(),
			SIGNAL(OnSignalUpdateMainOrder()), this, SLOT(OnUpdateMainOrder()));

	OnMESConfigRefurbish("");
}

MainQCCheckPage::~MainQCCheckPage()
{
	delete ui;
}

//上一页
void MainQCCheckPage::on_btPageLast_clicked()
{
	QScrollBar *scrollBar = ui->tableWidgetQCList->verticalScrollBar();
	int v = scrollBar->value();
	int end = v;
	end -= 9;
	if (end < 0)
	{
		end = 0;
	}
	for (int i = v; i >= end; i--)
	{
		scrollBar->setValue(i);
	}
}
//下一页
void MainQCCheckPage::on_btPageNext_clicked()
{
	QScrollBar *scrollBar = ui->tableWidgetQCList->verticalScrollBar();
	int v = scrollBar->value();
	v += 9;
	if (v > scrollBar->maximum())
	{
		v = scrollBar->maximum();
	}
	scrollBar->setValue(v);
}
//加载次品数据
void MainQCCheckPage::Loading()
{
	Order &m_order = OrderMainOperation::GetInstance().mainOrderCache;
	int type = 0;
	type = m_order.getMoBadTypeNo();
	m_MESTable.clear();
	//次品原因列表-次品原因配置文件
	QStringList qcNameList = AppInfo::bad_cfg.name[type];

	if (qcNameList.length() == 0)
	{
		ui->labelQCTitle->setText("次品原因(配置文件为空)");
	}
	else
	{
		ui->labelQCTitle->setText(QString("次品原因 共%1个").arg(qcNameList.size()));
	}

	for (int i = 0; i < m_order.orderBoyList.size(); i++)
	{
		MESTable table;
		MESTableItem item;

		//工单中的 次品原因总数 m_order.getMoBadReasonNum()
        if(m_order.getMoDispatchNo().length()>0 && m_order.getMoBadReasonNum()!= qcNameList.size())
        {
            logWarn(QString("次品原因总数不匹配，[%1]个数[%2],配置[%3]").arg(m_order.getMoDispatchNo()).arg(m_order.getMoBadReasonNum()).arg(qcNameList.size()));
            ToastDialog::Toast(NULL, "次品原因总数不匹配", 2000);

            ui->labelQCTitle->setText(QString("次品原因总数不匹配 配置%1个,工单%2个").arg(qcNameList.size()).arg(m_order.getMoBadReasonNum()));
        }

		for (int j = 0; j < qcNameList.size(); j++)
		{
			item.setIndex(j + 2);
			item.setName(qcNameList[j]);
			//不是空单 且 次品原因列表总长度  大于  工单中的原因总数
			if (m_order.getMoDispatchNo().length() > 0
					&& j >= m_order.getMoBadReasonNum())
			{
                //
				break;
			}
			if (j < m_order.orderBoyList[i].getPcsBadData().size())
				item.setValue(m_order.orderBoyList[i].getPcsBadData()[j]);
			else
				item.setValue(0);
			table.valueList.append(item);
		}
		m_MESTable.append(table);
	}
}

//上一件
void MainQCCheckPage::on_btUpItem_clicked()
{
	if (m_OrderIndex.BoyIndex > 0)
	{
		m_OrderIndex.BoyIndex--;
	}
	//显示工单
	showOrderInfo();
}

//下一件
void MainQCCheckPage::on_btNextItem_clicked()
{
	if (m_OrderIndex.BoyIndex < m_OrderIndex.BoySum - 1)
	{
		m_OrderIndex.BoyIndex++;
	}
	//显示工单
	showOrderInfo();
}

void MainQCCheckPage::OnMESConfigRefurbish(const QString& name)
{
	// 次品产生原因配置文件
	Loading();
	showOrderInfo();
}

void MainQCCheckPage::OnUpdateMainOrder()
{
	//当次品原因列表为空时,重新加载,会清除次品列表
	if (m_MESTable.size() == 0)
		Loading();
	//显示工单
	showOrderInfo();
}

void MainQCCheckPage::showOrderInfo()
{
	Order &m_order = OrderMainOperation::GetInstance().mainOrderCache;

	m_OrderIndex.BoySum = m_order.orderBoyList.size();

	//
	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;
	OrderMainOperation::OrderToQMap(m_order, orderMap, m_OrderIndex.BoyIndex,
			orderBoyMap);

	orderMap.unite(orderBoyMap);

	for (int i = 0; i < ui->tableWidgetOrderInfo->rowCount(); i++)
	{
		QString key = ui->tableWidgetOrderInfo->item(i, 0)->text();
		QString v = orderMap[key];
		if (ui->tableWidgetOrderInfo->item(i, 1) == NULL)
			ui->tableWidgetOrderInfo->setItem(i, 1, new QTableWidgetItem(v));
		else
			ui->tableWidgetOrderInfo->item(i, 1)->setText(v);
	}
	//显示次品列表
	showQCListInfo();
}

//显示次品列表
void MainQCCheckPage::showQCListInfo()
{
	ui->tableWidgetQCList->setColumnCount(4);
	if (m_MESTable.size() != 0 && m_MESTable.size() <= m_OrderIndex.BoyIndex) //件数 超出判断
		return;

	if (m_MESTable.size() == 0) //工单不存在时，只显示次品原因
	{
		QStringList qcNameList = AppInfo::bad_cfg.name[0];
		//重设定行数
		int rowC = qcNameList.size() / 4;
		if (qcNameList.size() % 4 > 0)
			rowC++;
		ui->tableWidgetQCList->setRowCount(rowC);

		int index = 0;
		if (qcNameList.size() > 0)
			for (int i = 0; i < ui->tableWidgetQCList->rowCount(); i++)
				for (int j = 0; j < QCLIST_ROW_MAX_COLUMN; j++)
				{
					if (index < qcNameList.size())
					{
						QTableWidgetItem *tabItem = new QTableWidgetItem(
								qcNameList[index]);
						tabItem->setTextAlignment(Qt::AlignCenter);
						ui->tableWidgetQCList->setItem(i, j, tabItem);
						index++;
					}
				}
	}
	else
	{
		//重设定行数
		int rowC = m_MESTable[m_OrderIndex.BoyIndex].valueList.size() / 4;
		if (m_MESTable[m_OrderIndex.BoyIndex].valueList.size() % 4 > 0)
			rowC++;
		ui->tableWidgetQCList->setRowCount(rowC);

		//总个数 6行 4列
		int index = 0;
		for (int i = 0; i < ui->tableWidgetQCList->rowCount(); i++)
			for (int j = 0; j < QCLIST_ROW_MAX_COLUMN; j++)
			{
				QString txt;
				if (index < m_MESTable[m_OrderIndex.BoyIndex].valueList.size())
				{
					MESTableItem &item =
							m_MESTable[m_OrderIndex.BoyIndex].valueList[index++];

					txt += item.getName();
					txt += "\r\n";
					txt += Tool::IntToQString(item.getValue());
					if (item.getInputValue() != 0)
					{
						txt += "+";
						txt += Tool::IntToQString(item.getInputValue());
					}
				}
				QTableWidgetItem *tabItem = new QTableWidgetItem(txt);
				tabItem->setTextAlignment(Qt::AlignCenter);
				ui->tableWidgetQCList->setItem(i, j, tabItem);
			}
	} //end show table widget
}

//用于修改Table的值 也就是次品原因
void MainQCCheckPage::on_tableWidgetQCList_itemClicked(QTableWidgetItem *item)
{
	int index = item->row() * QCLIST_ROW_MAX_COLUMN + item->column(); //行*最大列+当前列
			//当前件不超过 table大小， 当前选项不超过集合总数
	if (m_OrderIndex.BoyIndex >= m_MESTable.size()
			|| index >= m_MESTable[m_OrderIndex.BoyIndex].valueList.size())
		return;

	QString title =
			m_MESTable[m_OrderIndex.BoyIndex].valueList[index].getName();
	Order &m_order = OrderMainOperation::GetInstance().mainOrderCache;

	ToolTextInputDialog dialog(0, ToolTextInputDialog::typeNumber, "", title);
	if (dialog.exec() == ToolTextInputDialog::KeyEnter)
	{
		MESTableItem &vitem = m_MESTable[m_OrderIndex.BoyIndex].valueList[index];
		//判断:   输入无效，输入次品数大于本班良品数！
		int v = dialog.text().toInt();
		int sum = v;
		for (int i = 0; i < m_MESTable[m_OrderIndex.BoyIndex].valueList.size();
				i++)
		{
			sum +=
					m_MESTable[m_OrderIndex.BoyIndex].valueList[i].getInputValue();
		}
		//输入总和不能超过本班所剩余良品和
		sum +=
				m_order.orderBoyList[m_OrderIndex.BoyIndex].getCurClassPolishNo(); //本班打磨总数
		int okNum =
				m_order.orderBoyList[m_OrderIndex.BoyIndex].getCurClassTotalOkNum();
		if (sum > okNum)
		{
			QString str;
			str.sprintf("输入无效，输入次品数大于本班良品数！\r\n输入总和：%d(含打磨数:%d)\r\n本班良品:%d",
					sum,
					m_order.orderBoyList[m_OrderIndex.BoyIndex].getCurClassPolishNo(),
					okNum);
			MESMessageDlg::about(0, "提示", str);
			return;
		}
		vitem.setInputValue(v);
		showOrderInfo();
	}
}

void MainQCCheckPage::on_btSave_clicked()
{
	if (AppInfo::GetInstance().pd_func_cfg.NotSetKeyList.size() < 2)
	{
		MESMessageDlg::about(this, "提示", "功能配置文件未初始化,或不存在");
		return;
	}
	//判断是否有录入
	bool flag = false;
	for (int i = 0; i < m_MESTable.size(); i++)
	{
		for (int j = 0; j < m_MESTable[i].valueList.size(); j++)
		{
			if (m_MESTable[i].valueList[j].getInputValue() != 0)
				flag = true;
		}
	}
	if (!flag) //未录入数据时，也可以QC巡检
	{
		ToastDialog::Toast(this, "未录入数据", 1000);
		//ToastDialog::Toast(NULL, "未录入数据", 2000);
		//return;
	}
	ToolICScanDialog ic(AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(2)); //刷卡
	if (ic.exec() == QDialog::Rejected)
		return;

	DefectiveInfo info;

	Order &m_order = OrderMainOperation::GetInstance().mainOrderCache;

	info.setDispatchNo(m_order.getMoDispatchNo());   //	20	ASC	派工单号
	info.setDispatchPrior(m_order.getMoDispatchPrior()); //FDAT_DispatchPrior	30	ASC	派工项次
	info.setProcCode(m_order.getMoProcCode());   //FDAT_ProcCode	20	ASC	工序代码
	info.setStaCode(m_order.getMoStaCode()); //FDAT_StaCode	10	ASC	站别代码
	info.setCardId(ic.getICCardNo()); //FDAT_CardID	10	HEX	卡号
	info.setCardDate(Tool::GetCurrentDateTimeStr()); //FDAT_CardDate	6	HEX	次品数据产生时间
	info.setStatus(0); //FDAT_Status	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
	info.setMultiNum(m_MESTable.size()); //FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
	info.setBadReasonNum(m_order.getMoBadReasonNum()); //FDAT_BadReasonNum	1	HEX	次品原因总数M（0< M <= 100）
	info.setFdatBillType(0);	//单据类别(0表示次品,1表示隔离品)
	//每件产品的次品信息了
	for (int i = 0; i < m_MESTable.size(); i++)
	{
		DefectiveInfo::ProductInfo itemInfo;
		itemInfo.setItemNo(m_order.orderBoyList[i].getPcsItemNo());
		int sum = 0;
		QList<int> proItemList;
		for (int j = 0; j < m_MESTable[i].valueList.size(); j++)
		{
			MESTableItem &vitem = m_MESTable[i].valueList[j];
			itemInfo.getBadData().append(vitem.getInputValue());

			sum += vitem.getInputValue();
			proItemList.append(vitem.getSumValue());
		}
		info.getBadDataList().append(itemInfo);

		//为这件产品增加次品总数	本班次品数	每件次品数
		m_order.orderBoyList[i].OnAddPcsBadQty(sum);
		m_order.orderBoyList[i].OnAddCurClassBadNo(sum);
		m_order.orderBoyList[i].OnUpdatePCSBadData(proItemList);
	}

    if( Notebook("次品上传", info).insert() )
        ToastDialog::Toast(this, "保存成功", 1000);
    else
        ToastDialog::Toast(this, "保存失败,数据库保存出错.", 1000);
	{
		//重新加载...
		Loading();
		showOrderInfo();
        OrderMainOperation::GetInstance().OnUpdate();
	}
}
