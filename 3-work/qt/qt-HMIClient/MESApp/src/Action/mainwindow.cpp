#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextEdit>
#include <QLabel>
#include "plugin/mesmessagedlg.h"
#include "plugin/toastdialog.h"

MainWindow::MainWindow(QWidget *parent) :
		QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	page1 = new MainHomePage(this);
    page2 = new MainProductOrderPage;
    page3 = new MainQCCheckPage;
    page4 = new MainProcedureParamPage;
    page5 = new MainConfigPage;

	ui->stackedWidget->addWidget(page1);
    ui->stackedWidget->addWidget(page2);
    ui->stackedWidget->addWidget(page3);
    ui->stackedWidget->addWidget(page4);
    ui->stackedWidget->addWidget(page5);
	ui->stackedWidget->setCurrentIndex(0);

	animation = new QPropertyAnimation();

	connect(&CSCPAction::GetInstance(),
			SIGNAL(OnSignalCSCPConnect(bool,const QString&)), this,
			SLOT(CSCPConnectStatus(bool,const QString&)));

	connect(&CSCPAction::GetInstance(),
			SIGNAL(OnSignalMESMailMess(QString , QString , QString )), this,
			SLOT(OnMESMailMess(QString , QString , QString )));

	//主菜单放在左边
    on_btMenu_clicked();
    on_btUserInfo_clicked();

    startTimer(1000);
	Backlight::lightScren();

    logInfo("UI界面加载完成");
}

//系统通知
void MainWindow::OnMESMailMess(QString startT, QString endT, QString mess)
{
    noteList.clear();
    noteList.append(mess);
}

//显示通知
void MainWindow::on_SysMess_clicked()
{
    MESNoticeDialog *dialog=new MESNoticeDialog();
    if(noteList.size()>0)
        dialog->setText(noteList[0]);
    dialog->setGeometry(174,50,362,161);
    dialog->show();
}

void MainWindow::timerEvent(QTimerEvent *)
{
	static QPoint mousePoint(0, 0);
	static QDateTime curTime = QDateTime::currentDateTime();
	static bool changeFlag = false;
	static QDialog *showDialog = NULL;
	on_update_UI();

	//判断光标位置
	QPoint coursePoint = QCursor::pos(); //获取当前光标的位置
	if (mousePoint.x() != 0 && mousePoint.y() != 0)
	{
		if (coursePoint.x() != mousePoint.x()

				|| coursePoint.y() != mousePoint.y())
		{
			curTime = QDateTime::currentDateTime();
			if (changeFlag)
			{
				qDebug()<<"背光 点亮";
#if 1
				if(showDialog)
				{
					showDialog->close();
					delete showDialog;
					showDialog = NULL;
				}
#endif
				Backlight::lightScren();
				changeFlag = false;
			}
		}
		else
		{
			int n = curTime.secsTo(QDateTime::currentDateTime());
			if (AppInfo::GetInstance().getBackLightTime() > 1
					&& n > AppInfo::GetInstance().getBackLightTime()
					&& !changeFlag)
			{
				qDebug()<<"背光 关闭";
#if 1
				showDialog = new QDialog();
				showDialog->setStyleSheet("background-color: rgb(0, 0, 0);");
				showDialog->setGeometry(0, 0, 800, 480);
				showDialog->show();
#endif
				Backlight::closeScreen();
				changeFlag = true;
			}
		}
	}
	mousePoint = coursePoint;
}

//网络连接信息 flag状态标志, mess消息
void MainWindow::CSCPConnectStatus(bool flag, const QString &mess)
{
//	ui->listWidgetCSCP->insertItem(0, new QListWidgetItem(mess));
//	if (ui->listWidgetCSCP->count() > 17)
//		for (int i = 17; i < ui->listWidgetCSCP->count(); i++)
//			ui->listWidgetCSCP->removeItemWidget(ui->listWidgetCSCP->item(i));
}

void MainWindow::on_update_UI()
{
    ui->widgetMainTop->update();

//与服务器连接状态
	if (CSCPNotebookTask::GetInstance().getConnectStatusA()
			&& CSCPNotebookTask::GetInstance().getConnectStatusB())
        ui->CSCPStatus->setStyleSheet(
				"border-image: url(:/rc/res/Server_status.png);");
	else if (CSCPNotebookTask::GetInstance().getConnectStatusA())
        ui->CSCPStatus->setStyleSheet(
				"border-image: url(:/rc/res/Server_status_a.png);");
	else if (CSCPNotebookTask::GetInstance().getConnectStatusB())
        ui->CSCPStatus->setStyleSheet(
				"border-image: url(:/rc/res/Server_status_b.png);");
	else
        ui->CSCPStatus->setStyleSheet(
				"border-image: url(:/rc/res/Server_status_close.png);");

//无线信号
	int sig = unit::Tool::WirelessSignal();
	QString sigPng;
    sigPng.sprintf("border-image: url(:/rc/res/WiFi_status%d.png);", sig);
    ui->wifiSignal->setStyleSheet(sigPng);

	QDateTime dateTime = QDateTime::currentDateTime();
	QString time = Tool::DateTimeToQString(dateTime);

    ui->SysTime->setText(time);

//换班判断处理
	OrderMainOperation::GetInstance().OnReplaceClassProcess();
}

MainWindow::~MainWindow()
{
	delete page1;
	delete page2;
	delete page3;
	delete page4;
	delete animation;
	delete ui;
}

void MainWindow::on_bt_main_page1_clicked()
{
	ui->bt_main_page1->setChecked(true);
	ui->bt_main_page2->setChecked(false);
	ui->bt_main_page3->setChecked(false);
	ui->bt_main_page4->setChecked(false);
	ui->bt_main_page5->setChecked(false);
	ui->stackedWidget->setCurrentIndex(0);
    on_btMenu_clicked();
}

void MainWindow::on_bt_main_page2_clicked()
{
	ui->bt_main_page1->setChecked(false);
	ui->bt_main_page2->setChecked(true);
	ui->bt_main_page3->setChecked(false);
	ui->bt_main_page4->setChecked(false);
	ui->bt_main_page5->setChecked(false);
	ui->stackedWidget->setCurrentIndex(1);
    on_btMenu_clicked();
}

void MainWindow::on_bt_main_page3_clicked()
{
	ui->bt_main_page1->setChecked(false);
	ui->bt_main_page2->setChecked(false);
	ui->bt_main_page3->setChecked(true);
	ui->bt_main_page4->setChecked(false);
	ui->bt_main_page5->setChecked(false);
	ui->stackedWidget->setCurrentIndex(2);
    on_btMenu_clicked();
}

void MainWindow::on_bt_main_page4_clicked()
{
	ui->bt_main_page1->setChecked(false);
	ui->bt_main_page2->setChecked(false);
	ui->bt_main_page3->setChecked(false);
	ui->bt_main_page4->setChecked(true);
	ui->bt_main_page5->setChecked(false);
	ui->stackedWidget->setCurrentIndex(3);
    on_btMenu_clicked();
}

void MainWindow::on_bt_main_page5_clicked()
{
	/**
	 * 进入配置前须要刷卡
	 */
//1为管理员权限
	ToolICScanDialog ic(AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(1)); //刷卡
	if (ic.exec() == QDialog::Rejected)
	{
		ui->bt_main_page5->setChecked(false);
		return;
	}
	ui->bt_main_page1->setChecked(false);
	ui->bt_main_page2->setChecked(false);
	ui->bt_main_page3->setChecked(false);
	ui->bt_main_page4->setChecked(false);
	ui->bt_main_page5->setChecked(true);
	ui->stackedWidget->setCurrentIndex(4);
    on_btMenu_clicked();
}

void MainWindow::on_btMenu_clicked()
{
    QRect rect = ui->Menuframe->geometry();
    if (rect.x() < 0)
    {
        QRect des(0, rect.y(), 221, rect.height());
        ui->Menuframe->setGeometry(des);
    }
    else
    {
        int x = rect.width();
        QRect des(-x, rect.y(), 221, rect.height());
        ui->Menuframe->setGeometry(des);
    }
}

void MainWindow::on_btUserInfo_clicked()
{
    QRect rect=ui->widgetUserInfo->geometry();
    if(rect.x()<0)
    {
        QRect des(210, rect.y(), rect.width(), rect.height());
        ui->widgetUserInfo->setGeometry(des);
        //操作刷卡人信息显示
        QString str;

        Employee employee;
        if (Employee::queryIDCardNo(employee,
                AppInfo::GetInstance().getBrushCardId()))
        {
            ui->userName->setText("姓名: "+employee.getEmpNameCN());
            ui->userJob->setText("职位: "+employee.getPost());
            ui->userICNO->setText("卡号: "+AppInfo::GetInstance().getBrushCardId());
            ui->userWorkNo->setText("工号: "+employee.getEmpID());
        }
    } else {
       QRect des(-rect.width(), rect.y(), rect.width(), rect.height());
       ui->widgetUserInfo->setGeometry(des);
    }
}
