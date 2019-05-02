#include "mainprocedureparampage.h"
#include "ui_mainprocedureparampage.h"
#include "../Server/collection.h"
#include "plugin/productionmodeitem.h"

/**
 * 工艺参数界面:
 * 显示温度与生产模次信息界面
 * author:cj
 */
MainProcedureParamPage::MainProcedureParamPage(QWidget *parent) :
		QWidget(parent), ui(new Ui::MainProcedureParamPage)
{
	ui->setupUi(this);

	canvas = new QGraphicsScene(ui->graphicsView);
	canvas->setSceneRect(0, 0, ui->graphicsView->geometry().width() - 10,
			ui->graphicsView->geometry().height() - 10); //
	ui->graphicsView->setScene(canvas);

	connect(&OrderMainOperation::GetInstance(),
			SIGNAL(
					OnSignalModeNumberAdd(int , const QString , const QString , unsigned long , unsigned long , unsigned long )),
			this,
			SLOT(
					OnModeNumberAdd(int , const QString , const QString , unsigned long , unsigned long , unsigned long )));

	connect(&CSCPAction::GetInstance(),
			SIGNAL(OnSignalConfigRefurbish(const QString &)), this,
			SLOT(OnMESConfigRefurbish(const QString &)));
	Loading();

	//使用定时器显示温度
	startTimer(1000);
}

//配置文件更新后的信号接收
void MainProcedureParamPage::OnMESConfigRefurbish(const QString& str)
{
	Loading(); //重新加载数据
}

void MainProcedureParamPage::timerEvent(QTimerEvent *e)
{
	//读取温度并显示
	int channel = AppInfo::sys_func_cfg.temperateChannel();
	QList<int> list = Collection::GetInstance()->read_temperature_value(
			channel);
	for (int i = 0; i < itemList.size(); i++)
	{
		if (i < list.size())
			itemList[i]->setValue(list[i]);
		else
			itemList[i]->setValue(0);
	}

}

MainProcedureParamPage::~MainProcedureParamPage()
{
	qDeleteAll(itemList);
	itemList.clear();
	delete canvas;
	delete ui;
}

void MainProcedureParamPage::Loading()
{
	int channel = AppInfo::sys_func_cfg.temperateChannel(); //温度路数

	qDeleteAll(itemList);
	itemList.clear();
	canvas->clear();
	if(channel==0)
		return;
	//channel = 8;
	for (int i = 0; i < channel; i++)
	{
		QString str;
		str.sprintf("第%d路", i + 1);
		TemperatureItem * item = new TemperatureItem;
		item->setValue(100);
		item->setTitle(str);
		item->setPos(0, 0);
		//item->setFlag(QGraphicsItem::ItemIsMovable); //可以移动
		itemList.append(item);
	}
	//这里须要 进行位置平均分配
	int width = canvas->sceneRect().width();
	width = width / itemList.size(); //每个温度显示的总宽度

	for (int i = 0; i < itemList.size(); i++)
	{
		//垂直位置平均分配 itemList[i]->Size().width()
		//水平位置不变
		int y = canvas->sceneRect().height() / 2
				- itemList[i]->Size().height() / 2;
		int x = i * width;
		//width的中心位置显示温度条
		x += width / 2 - itemList[i]->Size().width() / 2;

		canvas->addItem(itemList[i]);
		itemList[i]->setPos(x, y);
	}
}

/**
 * 增加模次:
 * 模次,开始时间,结束时间, 机器周期, 填充周期,成型周期
 */
void MainProcedureParamPage::OnModeNumberAdd(int number,
		const QString startTime, const QString endTime,
		unsigned long machineCycle, unsigned long FillTime,
		unsigned long CycleTime)
{
	ProductionModeItem *modeItem = new ProductionModeItem(ui->listWidget);
	QListWidgetItem *item = new QListWidgetItem();
	modeItem->setContent(number, startTime, endTime, machineCycle, FillTime,
			CycleTime);
	item->setSizeHint(QSize(0, modeItem->geometry().height()));
	ui->listWidget->insertItem(0, item);
	ui->listWidget->setItemWidget(item, modeItem);
	ui->listWidget->clearFocus();
	ui->listWidget->clearSelection();
	ui->listWidget->count();
	QListWidgetItem *iii =ui->listWidget->takeItem(6);
	ui->listWidget->removeItemWidget(iii);
	delete iii;
    //ui->verticalScrollBar->setMaximum(ui->listWidget->verticalScrollBar()->maximum());
}

