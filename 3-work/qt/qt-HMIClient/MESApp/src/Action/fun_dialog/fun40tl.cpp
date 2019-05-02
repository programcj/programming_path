#include "fun40tl.h"
#include "ui_fun40tl.h"
#include "../plugin/mesmessagedlg.h"
#include "../tooltextinputdialog.h"

Fun40tl::Fun40tl(const QString &icCardId, QWidget *parent) :
		QDialog(parent, Qt::FramelessWindowHint), ui(new Ui::Fun40tl)
{
	ui->setupUi(this);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理
	m_icCardId = icCardId;

    IndexMaterial=0;
    LoadMateralInfo();
	ShowOrderInfo();
}

Fun40tl::~Fun40tl()
{
	delete ui;
}

void Fun40tl::on_btExit_clicked()
{
	close();
}

void Fun40tl::ShowOrderInfo()
{
	const Order &mainOrder = OrderMainOperation::GetInstance().mainOrderCache;

	QMap<QString, QString> orderMap;
	QMap<QString, QString> orderBoyMap;
	QMap<QString, QString> materialMap;
	OrderMainOperation::OrderToQMap(mainOrder, orderMap, 0, orderBoyMap);

    if(IndexMaterial<materialList.size())
        OrderMainOperation::MaterialToMap(materialList.at(IndexMaterial), materialMap);

	orderMap.unite(orderBoyMap);
	orderMap.unite(materialMap);

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
}

void Fun40tl::LoadMateralInfo()
{
    const Order &mainOrder = OrderMainOperation::GetInstance().mainOrderCache;
    if (Material::queryAll(materialList, mainOrder.getMoDispatchNo()))
    {

    }
}

//录入数据
void Fun40tl::on_tableWidgetValue_itemClicked(QTableWidgetItem *item)
{
	QTableWidgetItem *nameItem = ui->tableWidgetValue->item(item->row(), 0);
	if (nameItem == NULL)
		return;
	ToolTextInputDialog dialog(NULL, ToolTextInputDialog::none, "",
			nameItem->text());
	if (dialog.exec() == ToolTextInputDialog::KeyHide)
		return;

	ui->tableWidgetValue->setItem(item->row(), 1,
			new QTableWidgetItem(dialog.text()));
}

//保存,添加这个料 先添加到 原料表,再保存到数据上传表
void Fun40tl::on_btSave_clicked()
{
	for (int i = 0; i < ui->tableWidgetValue->rowCount(); i++)
	{
		if (ui->tableWidgetValue->item(i, 1) == NULL)
		{
			MESMessageDlg::about(this, "提示",
					"未录入" + ui->tableWidgetValue->item(i, 0)->text());
			return;
		}
	}

	Material material;
	material.setDispatchNo(
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo()); //派工单号
	material.setMaterialNo(ui->tableWidgetValue->item(0, 1)->text()); //原料编号
	material.setMaterialName(ui->tableWidgetValue->item(1, 1)->text()); //原料名称
	//以下与投料有关
	material.setBatchNo(ui->tableWidgetValue->item(2, 1)->text()); //原料批次号
	material.setFeedingQty(ui->tableWidgetValue->item(3, 1)->text()); //原料投料数量
	material.setFeedingTime(Tool::GetCurrentDateTimeStr()); //原料投料时间
	if (!material.subimt())
	{
		ToastDialog::Toast(NULL,"保存出错",1000);
		return;
	}

	OtherSetInfo info;
	OtherSetInfo::AdjMaterial adj;
	adj.DispatchNo =
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo();	//	20	ASC	派工单号
	adj.CardID = m_icCardId;	//	10	ASC	员工卡号

	QList<Material> list;
	if (Material::queryAll(list,
			OrderMainOperation::GetInstance().mainOrderCache.getMoDispatchNo()))
	{
		for (int i = 0; i < list.size(); i++)
		{
			OtherSetInfo::ItemMaterial item;
			Material &mater = list[i];
			item.MaterialNO = mater.getMaterialNo();	//	50	ASC	原料1编号
			item.MaterialName = mater.getMaterialName();	//	100	ASC	原料1名称
			item.BatchNO = mater.getBatchNo();	//	50	ASC	原料1批次号
			item.FeedingQty = mater.getFeedingQty().toInt();//	4	HEX	原料1投料数量
			item.FeedingTime = mater.getFeedingTime();	//	6	HEX	原料1投料时间
			adj.MaterialList.append(item);
		}
	}
	info.setAdjMaterial(adj);

	Notebook("投料", info).insert();

	ToastDialog::Toast(NULL,"保存成功",1000);
	ShowOrderInfo();
}

//上一料
void Fun40tl::on_btUp_clicked()
{
    LoadMateralInfo();
    if(IndexMaterial>0)
        IndexMaterial--;
    ShowOrderInfo();
}

//下一料
void Fun40tl::on_btNext_clicked()
{
    LoadMateralInfo();
    if(IndexMaterial<materialList.size()-1)
        IndexMaterial++;
    ShowOrderInfo();

}
