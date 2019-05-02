#include "syncorderdialog.h"
#include "ui_syncorderdialog.h"

SyncOrderDialog::SyncOrderDialog(QWidget *parent) :
		QDialog(parent), ui(new Ui::SyncOrderDialog)
{
	ui->setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	if (!connect(MESNet::getInstance(),
			SIGNAL(sig_protocol_other(const MESOtherResponse&)), this,
			SLOT(slot_protocol_other_sync_order(const MESOtherResponse&))))
	{
	}

	syncSubimt = false;
	flag = false;
	//timer.setSingleShot(true); //只调用一次
	QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(_q_time_Start()));
	timer.start(500);
	ui->progressBar->setRange(0, 100);
	ui->progressBar->setValue(0);
}

SyncOrderDialog::~SyncOrderDialog()
{
	delete ui;
}

void SyncOrderDialog::slot_protocol_other_sync_order(
		const MESOtherResponse &response)
{
	if (response.FDAT_DataType != OtherSetInfo::AsAsyncOrder)
		return;
	orderIndexList.append(response.orderIndexList); //服务器工单列表

	QList<Order> myList; //本地工单
	Order::queryAll(myList);
	//删除本地多出的工单
	for (int i = 0; i < myList.size(); i++)
	{
		int j = 0;
		for (j = 0; j < orderIndexList.size(); j++)
		{
			if (myList[i].getMoDispatchNo()
					== orderIndexList[j].FDAT_DispatchNo)
				break;
		}

		logInfo(
				QString("本地工单列表: %1,%2").arg(myList[i].getMainOrderFlag()).arg(
						myList[i].getMoDispatchNo()));
		if (j == orderIndexList.size())
		{
			logInfo("--删除");
			myList[i].remove();
		}
	}

	flag = true;
}

void SyncOrderDialog::_q_time_Start()
{
	if (!syncSubimt)
	{
		CSCPNotebookTask::GetInstance().OnAsyncOrder(); //请求同步工单
		syncSubimt = true;
		return;
	}

	if (!flag) //是否响应了
		return;

	if (orderIndexList.size() == 0)
		accept();

	int serCount = orderIndexList.size();
	int myCount = Order::count();

	ui->progressBar->setValue((myCount * 1.0 / serCount * 1.0) * 100);

	if (orderIndexList.size() == Order::count())
	{
		//更新本地工单的排序号
		//显示对话框 提示同步的过程
		for (int i = 0; i < orderIndexList.size(); i++)
		{
			logDebug(
					QString("服务器工单列表:%1,%2").arg(
							orderIndexList[i].FDAT_SerialNumber).arg(
							orderIndexList[i].FDAT_DispatchNo));

			Order::UpdateMainOrderFlag(orderIndexList[i].FDAT_DispatchNo,
					orderIndexList[i].FDAT_SerialNumber);
		}
		accept();
	}
}

void SyncOrderDialog::on_btExit_clicked()
{
	//qDebug() << "on_btExit_clicked";
	timer.stop();

//    accept(); // accept（）（返回QDialog::Accepted）
//    reject（）;//（返回QDialog::Rejected），
//    done（int r）;//（返回r），
//    close（）;//（返回QDialog::Rejected），
//    hide（）;//（返回 QDialog::Rejected），
//    destory（）;//（返回QDialog::Rejected）

	close();
}

