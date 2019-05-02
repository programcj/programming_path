#include "fun36tj.h"
#include "ui_fun36tj.h"
#include "../plugin/mesmessagedlg.h"

/**
 * 调机
 *
 * 次品数不记录到 本工单的次品中,良品空模数，空模总数
 * 数据保存后再次录入，是在以前的机础上增加；
 * 调机有件的区别
 */
Fun36tj::Fun36tj(ButtonStatus *btStatus, QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::fun36tj)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	ui->tableWidget->setVerticalScrollBar(ui->verticalScrollBar);
	this->btStatus = btStatus;

	ui->comboBox->addItem("辅设故障", 24);
	ui->comboBox->addItem("机器故障", 25);
	ui->comboBox->addItem("模具故障", 26);
	ui->comboBox->addItem("工艺调机", 36);
	ui->comboBox->addItem("试模", 44);
	ui->comboBox->addItem("试料", 47);
}

Fun36tj::~Fun36tj()
{
	delete ui;
}
//退出
void Fun36tj::on_btExit_clicked()
{
	close();
}

//开始调机
void Fun36tj::on_btStart_clicked()
{
	if (btStatus->isShowStart())
	{
		MESMessageDlg::about(this, "提示", "己开始");
		return;
	}
	//刷卡
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
	btStatus->setBindText(ic.getICCardNo());

	//刷卡数据携带长度为0
	BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache,
			ic.getICCardNo(), btStatus->getKeyInfo()->funIndex,
			Tool::GetCurrentDateTimeStr(), 0);
	Notebook(btStatus->name + "开始刷卡", brush).insert();
	btStatus->setShowStartCard(); //显示为开始卡
	OrderInfoShow();
}
//结束调机
void Fun36tj::on_btClose_clicked()
{
	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(this, "提示", "己结束");
		return;
	}
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;
	btStatus->setBindText("");
	BrushCard brush(OrderMainOperation::GetInstance().mainOrderCache,
			ic.getICCardNo(), btStatus->getKeyInfo()->funIndex,
			Tool::GetCurrentDateTimeStr(), 1);
	Notebook(btStatus->name + "结束刷卡", brush).insert();
	{
		//清除主单的 良品总数 + 空模总数 + 调机的 次品原因
		for (int i = 0;
				i
						< OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size();
				i++)
		{
            OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].OnUpdateADJOKNum(
					0);
            OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].OnUpdateADJEmptyMoldNum(
					0);
			OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].OnUpdateADJDefList(
					"0");
			OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].OnUpdateADJDefCount(
					0);

		}
	}
	btStatus->setShowEndCard();
	//清空
	Loading();
	OrderInfoShow();
}

//保存
void Fun36tj::on_btSave_clicked()
{
	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(this, "提示", "未开始");
		return;
	}
	if (OrderMainOperation::GetInstance().mainBoyIndex >= m_MESTable.size())
	{
		MESMessageDlg::about(this, "提示", "数据出错");
		return;
	}
	ToolICScanDialog ic(btStatus->getKeyInfo());
	if (ic.exec() == QDialog::Rejected)
		return;

	// 产生调机记录信息
	ADJMachine adjInfo;
	adjInfo.setDispatchNo(
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo());
	adjInfo.setDispatchPrior(
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchPrior()); //	30	ASC	派工项次
	adjInfo.setProcCode(
			OrderMainOperation::GetInstance().mainOrderCache.getMoProcCode()); //	20	ASC	工序代码
	adjInfo.setStaCode(
			OrderMainOperation::GetInstance().mainOrderCache.getMoStaCode()); //	10	ASC	站别代码
	adjInfo.setStartCardId(btStatus->getBindText()); //	10	ASC	开始调机卡号
	adjInfo.setStartTime(Tool::GetCurrentDateTimeStr()); //	6	HEX	开始调机时间
	adjInfo.setEndCardId(ic.getICCardNo()); //	10	ASC	结束调机卡号
	adjInfo.setEndTime(Tool::GetCurrentDateTimeStr()); //	6	HEX	结束调机时间
	adjInfo.setCardType(
			ui->comboBox->itemData(ui->comboBox->currentIndex()).toInt()); //	1	HEX	调机原因编号
	adjInfo.setMultiNum(
			OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size()); //	1	HEX	总件数N（0< N <= 100）

	for (int i = 0; i < m_MESTable.size(); i++)
	{
		ADJMachine::AdjustData item;
		item.ItemNO =
				OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].getPcsItemNo();
		item.CurChangeQty = m_MESTable[i].valueList[0].getInputValue(); //	4	DWORD	本次调机良品总数
		item.NullQty = m_MESTable[i].valueList[1].getInputValue(); //	4	DWORD	本次调机空模总数
		item.ProdQty = 0; //	4	DWORD	本次调机生产总数
		adjInfo.getAdjstDataList().append(item);

		OrderMainOperation::GetInstance().OnAddADJOKNum(i,
				m_MESTable[i].valueList[0].getInputValue());
		OrderMainOperation::GetInstance().OnAddADJEmptyMoldNum(i,
				m_MESTable[i].valueList[1].getInputValue());

		m_MESTable[i].valueList[0].Save();
		m_MESTable[i].valueList[1].Save();
	}
	//保存到数据库哦..
	Notebook("产生调机记录信息", adjInfo).insert();

	// 产生次品记录信息 次品功能码为调机功能码
	DefectiveInfo info;
	DefectiveInfo::ProductInfo itemInfo;

	info.setDispatchNo(
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo()); //	20	ASC	派工单号
	info.setDispatchPrior(
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchPrior()); //FDAT_DispatchPrior	30	ASC	派工项次
	info.setProcCode(
			OrderMainOperation::GetInstance().mainOrderCache.getMoProcCode()); //FDAT_ProcCode	20	ASC	工序代码
	info.setStaCode(
			OrderMainOperation::GetInstance().mainOrderCache.getMoStaCode()); //FDAT_StaCode	10	ASC	站别代码
	info.setCardId(btStatus->getBindText()); //FDAT_CardID	10	HEX	卡号
	info.setCardDate(Tool::GetCurrentDateTimeStr()); //FDAT_CardDate	6	HEX	次品数据产生时间
	info.setStatus(btStatus->funIndex); //FDAT_Status	1	HEX	录入次品类别:功能代号(如试模44) 详见配置文件功能类别
	info.setMultiNum(
			OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size()); //FDAT_MultiNum	1	HEX	总件数N（0< N <= 100）
	info.setBadReasonNum(
			OrderMainOperation::GetInstance().mainOrderCache.getMoBadReasonNum()); //FDAT_BadReasonNum	1	HEX	次品原因总数M（0< M <= 100）
	info.setFdatBillType(0); //

	for (int i = 0; i < m_MESTable.size(); i++)
	{
		int DefectCount = 0;
		itemInfo.setItemNo(
				OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].getPcsItemNo());
		QString str1 = "";
		for (int j = 0; j < m_MESTable[i].valueList.size() - 2; j++)
		{
			QString temp;
			itemInfo.getBadData().append(
					m_MESTable[i].valueList[j + 2].getInputValue());
			DefectCount += m_MESTable[i].valueList[j + 2].getInputValue(); //增加到次品总数中

			m_MESTable[i].valueList[j + 2].Save();

			temp.sprintf("%d", m_MESTable[i].valueList[j + 2].getValue());
			str1 += temp;
			if (j < m_MESTable[i].valueList.size() - 2 - 1)
				str1 += ",";
		}

		logDebug(QString("保存:%1").arg(str1));

		OrderMainOperation::GetInstance().OnUpdateADJDefList(i, str1); //更新到数据库中
		//添加 调机次品数
		OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].OnAddADJDefCount(
				DefectCount);
	}
	info.getBadDataList().append(itemInfo);

	Notebook("次品上传", info).insert();
	ToastDialog::Toast(NULL, "保存成功", 1000);
	OrderMainOperation::GetInstance().OnUpdate();
	OrderInfoShow();
}

//上一件
void Fun36tj::on_btItemUp_clicked()
{
	if (OrderBoyIndex > 0)
		OrderBoyIndex--;
	OrderInfoShow();
}
//下一件
void Fun36tj::on_btItemNext_clicked()
{
	if (OrderBoyIndex + 1
			< OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size())
		OrderBoyIndex++;
	OrderInfoShow();
}

//加载数据
bool Fun36tj::Loading()
{
	int size = 0;
	int type = 0;

	OrderBoyIndex = 0;
	type = OrderMainOperation::GetInstance().mainOrderCache.getMoBadTypeNo(); //次品类型选项（配置文件中）
	size = AppInfo::bad_cfg.name[type].size(); //60、	次品原因配置文件：bad_cfg.bin
	if (size == 0)
	{
		//没有次品原因配置
		ToastDialog::Toast(NULL, "没有找到次品原因", 2000);
		return false;
	}

	size += 2;
	m_MESTable.clear();

	for (int i = 0;
			i
					< OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size();
			i++)
	{
		MESTable table;
		MESTableItem item;
		item.setIndex(0);
		item.setName("良品总数");
		item.setValue(
				OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].getADJOKNum());

		table.valueList.append(item);
		item.setIndex(1);
		item.setName("空模总数");
		item.setValue(
				OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].getADJEmptyMoldNum());
		table.valueList.append(item);

		QString noText =
				OrderMainOperation::GetInstance().mainOrderCache.orderBoyList[i].getADJDefList();

		QStringList list = noText.split(',');

		for (int j = 0; j < size - 2; j++)
		{
			item.setIndex(j + 2);
			item.setName(AppInfo::bad_cfg.name[type][j]);

			if (j < list.size())
				item.setValue(list[j].length() == 0 ? 0 : list[j].toInt());
			else
				item.setValue(0);

			table.valueList.append(item);
		}

		m_MESTable.append(table);
	}
    if(OrderMainOperation::GetInstance().mainOrderCache.orderBoyList.size()==0)
    {
        MESMessageDlg::about(this, "提示", "没有生产工单");
        return false;
    }
	return true;
}

void Fun36tj::showEvent(QShowEvent* e)
{
	OrderInfoShow();
}

//显示
void Fun36tj::OrderInfoShow()
{
    //工单件号>
	if (OrderBoyIndex >= m_MESTable.size())
    {
        MESMessageDlg::about(this, "提示", "数据出错");
		return;
	}
	int size = m_MESTable[OrderBoyIndex].valueList.size();
	ui->tableWidget->clear();
	ui->tableWidget->setColumnCount(2);
	ui->tableWidget->setRowCount(size);
	for (int i = 0; i < size; i++)
	{
		QString v;
		v.sprintf("%d", m_MESTable[OrderBoyIndex].valueList[i].getSumValue());

		ui->tableWidget->setItem(i, 0,
				new QTableWidgetItem(
						m_MESTable[OrderBoyIndex].valueList[i].getName()));
		ui->tableWidget->setItem(i, 1, new QTableWidgetItem(v));
	}

	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;
	OrderMainOperation::OrderToQMap(
			OrderMainOperation::GetInstance().mainOrderCache, orderMap,
			OrderBoyIndex, orderBoyMap);

	for (int i = 0; i < 2; i++)
	{
		QString key = ui->tableWidgetInfo->item(i, 0)->text();
		QString v = orderMap[key];
		ui->tableWidgetInfo->item(i, 1)->setText(v);
	}

	for (int i = 2; i < 9; i++)
	{
		QString key = ui->tableWidgetInfo->item(i, 0)->text();
		QString v = orderBoyMap[key];
		ui->tableWidgetInfo->item(i, 1)->setText(v);
	}
	ui->tableWidgetInfo->setItem(9, 0, new QTableWidgetItem("调机状态"));
	if (!btStatus->isShowStart())
		ui->tableWidgetInfo->setItem(9, 1, new QTableWidgetItem("未调机"));
	else
		ui->tableWidgetInfo->setItem(9, 1, new QTableWidgetItem("调机开始"));

	ui->verticalScrollBar->setValue(0);
	ui->verticalScrollBar->setMinimum(ui->tableWidget->verticalScrollBar()->minimum());
	ui->verticalScrollBar->setMaximum(ui->tableWidget->verticalScrollBar()->maximum());
}

void Fun36tj::on_verticalScrollBar_valueChanged(int value)
{
	ui->tableWidget->verticalScrollBar()->setValue(value);
}

//录入数量
void Fun36tj::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
	if (btStatus->isShowEnd())
	{
		MESMessageDlg::about(this, "提示", "未开始");
		return;
	}

    ToolTextInputDialog inputDialog(this, ToolTextInputDialog::typeNumber,"","录入数量"); //弹出输入框输入
	if (inputDialog.exec() == ToolTextInputDialog::KeyHide)
	{
		return;
	}
	if (OrderBoyIndex >= m_MESTable.size())
	{
		MESMessageDlg::about(this, "提示", "数据出错");
		return;
	}
	m_MESTable[OrderBoyIndex].valueList[item->row()].setInputValue(
			inputDialog.text().toInt());
	OrderInfoShow();
}

