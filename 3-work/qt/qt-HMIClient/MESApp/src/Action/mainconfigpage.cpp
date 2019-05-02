#include "mainconfigpage.h"
#include "ui_mainconfigpage.h"
#include "wifidialog.h"
#include <QMessageBox>
#include <QFile>

#include "networkinfo/netconfig.h"
#include "cmdtool/cmdrundialog.h"
#include "inputtime/inputtime.h"
#include "tempcalibrate/tempecalibrate.h"
#include "../Public/public.h"
#include "../Action/tooltextinputdialog.h"
#include "../Server/rfserver.h"
#include "../Server/collection.h"
#include "../Server/NetCommandService.h"
#include "plugin/mesmessagedlg.h"
#include "../application.h"

static QMap<QString, QString> netinfoMap;
static QMap<QString, QString> sysinfoMap;
static QMap<QString, QString> switchMap;
MainConfigPage::MainConfigPage(QWidget *parent) :
		QWidget(parent), ui(new Ui::MainConfigPage)
{
	ui->setupUi(this);
	dirpath = QApplication::applicationDirPath()+ "/config_file";

	//ui->tw_file->setcolumnwidth(0,300);

#ifndef _WIN32
	initNetMap("eth0");
	initNetMap("wlan0");
#endif
	ui->tw_file->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}");
	ui->tw_file->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tw_file->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tw_file->setStyleSheet("selection-background-color:lightblue;");
	ui->tw_file->verticalHeader()->setVisible(false);
	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this,
			SLOT(handleTabPress(int)));
	ui->tabWidget->setCurrentIndex(1);
	sysInfoMapFun();
	switchInfoMapFun();
	init_button_state();
	init_deviceInfo();
	init_sysInfo();
	init_switchInfo();

}

MainConfigPage::~MainConfigPage()
{
	delete ui;
}

void MainConfigPage::on_btExit_clicked()
{
//	//关闭数据库和网络
//
//	CSCPNotebookTask::GetInstance().closeCSCPServer();
//	SQLiteBaseHelper::getInstance().close();
//	SQLiteProductedHelper::getInstance().close();

	if (QDialogButtonBox::Ok
			== MESMessageDlg::information(NULL, "提示", "确定要退出程序吗",
					QDialogButtonBox::Ok | QDialogButtonBox::No,
					QDialogButtonBox::Ok))
	{
		Application::instance()->MESExit();
	}
}

void MainConfigPage::on_btWifi_clicked()
{
	proc = new QProcess(this);
	this->hide();
	proc->start("/bin/wpa_gui -qws");
	// proc->start("/usr/bin/ts_calibrate");
	proc->waitForFinished(-1);
	proc->close();
	delete proc;
	//this->repaint();
	emit exitprocess();
	this->show();

}
void MainConfigPage::on_bt_calib_clicked()
{
#ifndef _WIN32
	proc = new QProcess(this);
	this->hide();
	system("rm -rf /etc/pointercal");
	proc->start("/usr/bin/ts_calibrate");
	proc->waitForFinished(-1);
	proc->close();
	delete proc;
	emit exitprocess();
	this->show();
#endif
}
void MainConfigPage::on_btDevID_clicked()
{
	ToolTextInputDialog passDialog(this, ToolTextInputDialog::typeText,
			AppInfo::GetInstance().getDevId(), "设备配置");
	if (passDialog.exec() == ToolTextInputDialog::KeyHide)
	{
		return;
	}
	if (passDialog.text().length() == 0)
	{
		QMessageBox::about(this, "提示", "数据出错");
		return;
	}
	//保存配置
	AppInfo::GetInstance().setDevId(passDialog.text());
	AppInfo::GetInstance().saveConfig();
}
void MainConfigPage::on_btMachineID_clicked()
{
	ToolTextInputDialog passDialog(this, ToolTextInputDialog::typeText,
			AppInfo::GetInstance().getMachineId(), "机器配置");
	if (passDialog.exec() == ToolTextInputDialog::KeyHide)
	{
		return;
	}
	if (passDialog.text().length() == 0)
	{
		QMessageBox::about(this, "提示", "数据出错");
		return;
	}
	//保存配置
	AppInfo::GetInstance().setMachineId(passDialog.text());
	AppInfo::GetInstance().saveConfig();
}

void MainConfigPage::on_btNetconfig_clicked()
{
	netconfig config_dlg;
	config_dlg.setWindowModality(Qt::WindowModal);
	//config_dlg.setAttribute (Qt::WA_DeleteOnClose);

	config_dlg.exec();

}
void MainConfigPage::on_btCardPass_clicked()
{
	ToolTextInputDialog passDialog(this, ToolTextInputDialog::typeText,
			AppInfo::GetInstance().getBrushCardPassword(), "设置刷卡密码");
	if (passDialog.exec() == ToolTextInputDialog::KeyHide)
	{
		return;
	}
	if (passDialog.text().length() == 0)
	{
		QMessageBox::about(this, "提示", "数据出错");
		return;
	}
	AppInfo::GetInstance().setBrushCardPassword(passDialog.text());
	AppInfo::GetInstance().saveConfig();
	MESMessageDlg::about(NULL, "设置刷卡密码", "更新成功");
}

void MainConfigPage::on_btSystemTime_clicked()
{
	InputTime checktimeDlg;
//  checktimeDlg.setWindowModality(Qt::WindowModal);
//  checktimeDlg.setWindowFlags(Qt::FramelessWindowHint);
	int ret = checktimeDlg.exec();

	if (ret == QDialog::Accepted)
	{
		QDateTime time;
		QString str = checktimeDlg.getEditTimeText();
		str.insert(0, "\"");
		str.append("\"");
		str.insert(0, "date -s");
		qDebug() << str;
		{
			//判断格式是否正确
#ifndef win32
			system(str.toLatin1().data());

			//强制写入到CMOS
			system("hwclock -w");
			MESMessageDlg::about(this, "信息", "时间配置成功！");
#endif
		}

	}
	else if (ret == QDialog::Rejected)
	{
		return;
	}

}

void MainConfigPage::on_btTempAdjust_clicked()
{
	tempecalibrate tempDlg(this);
	tempDlg.setWindowModality(Qt::WindowModal);
	tempDlg.setWindowFlags(Qt::FramelessWindowHint);
	int ret = tempDlg.exec();
	if (ret == QDialog::Accepted)
	{
		if (Collection::GetInstance()->setTempCalibre(
				tempDlg.getCombChannelValue(),
				tempDlg.getEditTempValue().toInt()))
		{
			MESMessageDlg::about(this, "信息", "温度校准成功！");
		}
		else
		{
			MESMessageDlg::about(this, "信息", "温度校准失败！");
		}
	}
	else if (ret == QDialog::Rejected)
	{
		return;
	}
}

void MainConfigPage::on_btNetTest_clicked()
{
	cmdrundialog config_dlg;
	config_dlg.setWindowModality(Qt::WindowModal);

	config_dlg.exec();
}

void MainConfigPage::on_btRestart_clicked()
{
	if (QDialogButtonBox::Ok
			== MESMessageDlg::information(NULL, "提示", "确定要退出程序吗",
					QDialogButtonBox::Ok | QDialogButtonBox::No,
					QDialogButtonBox::Ok))
	{
		//system("reboot");
		if(IpcMessage::sendMsgRestartApp())
		{
			Application::instance()->MESExit();
		}
	}
}

void MainConfigPage::on_btReboot_clicked()
{
	if (QDialogButtonBox::Ok
				== MESMessageDlg::information(NULL, "提示", "确定要重启系统吗",
						QDialogButtonBox::Ok | QDialogButtonBox::No,
						QDialogButtonBox::Ok))
		{
			Collection::GetInstance()->ColetctStop();
			RFIDServer::GetInstance()->RFIDEnd();
			//数据采集处理
			CollectCycle::GetInstance()->interrupt();
			CollectCycle::GetInstance()->quit();

			logInfo("系统退出");
			//关闭数据库和网络接口
			SQLiteBaseHelper::getInstance().close();
			SQLiteProductedHelper::getInstance().close();
            CSCPNotebookTask::GetInstance().closeCSCPServer();
			system("reboot");
		}
}

void MainConfigPage::init_deviceInfo()
{
	QString v;
	ui->tw_addr->horizontalHeader()->resizeSection(1, 100);
	for (int i = 0; i < ui->tw_addr->rowCount(); i++)
	{
		QString key = ui->tw_addr->item(i, 0)->text();
#ifndef _WIN32

		v = netinfoMap[key];
#else
		v= "测试";
#endif

		if (ui->tw_addr->item(i, 1) == NULL)
		{
			ui->tw_addr->setItem(i, 1, new QTableWidgetItem(v));
		}
		else
			ui->tw_addr->item(i, 1)->setText(v);
	}

	ui->tw_addr->resizeColumnsToContents(); //根据内容调整列宽
	ui->tw_addr->resizeColumnToContents(1); //根据内容自动调整给定列宽

	ui->tw_addr->horizontalHeader()->setResizeMode(QHeaderView::Stretch); //使列完全填充并平分

	ui->tw_addr->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
}

void MainConfigPage::init_sysInfo()
{
	QString v;
	ui->tableWidget->horizontalHeader()->resizeSection(1, 100);
	for (int i = 0; i < ui->tw_sys->rowCount(); i++)
	{
		QString key = ui->tw_sys->item(i, 0)->text();

//#ifndef _WIN32
		v = sysinfoMap[key];
//#else
//		v= "测试";
//#endif
		if (ui->tw_sys->item(i, 1) == NULL)
		{
			ui->tw_sys->setItem(i, 1, new QTableWidgetItem(v));
		}
		else
			ui->tw_sys->item(i, 1)->setText(v);
	}

	ui->tw_sys->resizeColumnsToContents(); //根据内容调整列宽
	ui->tw_sys->resizeColumnToContents(1); //根据内容自动调整给定列宽

	ui->tw_sys->horizontalHeader()->setResizeMode(QHeaderView::Stretch); //使列完全填充并平分

	ui->tw_sys->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
}

void MainConfigPage::init_switchInfo()
{
	QString v;
	ui->tableWidget->horizontalHeader()->resizeSection(1, 100);
	for (int i = 0; i < ui->tw_ctr->rowCount(); i++)
	{
		QString key = ui->tw_ctr->item(i, 0)->text();
		v = switchMap[key];
		/*#ifndef _WIN32
		 v = switchMap[key];
		 #else
		 v= "测试";
		 #endif*/
		if (ui->tw_ctr->item(i, 1) == NULL)
		{
			ui->tw_ctr->setItem(i, 1, new QTableWidgetItem(v));
		}
		else
			ui->tw_ctr->item(i, 1)->setText(v);
	}

	ui->tw_ctr->resizeColumnsToContents(); //根据内容调整列宽
	ui->tw_ctr->resizeColumnToContents(1); //根据内容自动调整给定列宽

	ui->tw_ctr->horizontalHeader()->setResizeMode(QHeaderView::Stretch); //使列完全填充并平分

	ui->tw_ctr->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
}

void MainConfigPage::initNetMap(QString name)
{
	//netinfoMap.clear();
	QFile f("/etc/network/.conf/" + name);
	if (f.open(QFile::ReadOnly))
	{
		QTextStream t(&f);
		QString str = t.readLine();
		if (str == "static")
		{
			if (name == "wlan0")
			{
				// i->serverip =t.readLine();
				netinfoMap.insert("无线IP", t.readLine());
				netinfoMap.insert("无线掩码", t.readLine());
				netinfoMap.insert("无线网关", t.readLine());
				if(AppInfo::GetInstance().getNetDeviceName() == name)
				{
					netinfoMap.insert("服务器IP", t.readLine());
					netinfoMap.insert("服务器端口A", t.readLine());
					netinfoMap.insert("服务器端口B", t.readLine());
				}
			}
			if (name == "eth0")
			{
				netinfoMap.insert("以太网IP", t.readLine());
				netinfoMap.insert("以太网掩码", t.readLine());
				netinfoMap.insert("以太网网关", t.readLine());
				if(AppInfo::GetInstance().getNetDeviceName() == name)
				{
					netinfoMap.insert("服务器IP", t.readLine());
					netinfoMap.insert("服务器端口A", t.readLine());
					netinfoMap.insert("服务器端口B", t.readLine());
				}
			}
		}
		f.close();
	}
}

void MainConfigPage::sysInfoMapFun()
{
	sysinfoMap.clear();
	sysinfoMap.insert("软件版本", AppInfo::GetInstance().getVersion());
	sysinfoMap.insert("内核版本", "3.2.0");
	sysinfoMap.insert("机器ID", AppInfo::GetInstance().getMachineId());
	sysinfoMap.insert("设备ID", AppInfo::GetInstance().getDevId());
	sysinfoMap.insert("通信密码", AppInfo::GetInstance().getBrushCardPassword());
	if (AppInfo::sys_func_cfg.machineType() == 0)
	{
		sysinfoMap.insert("机器类型", "普通机型");
	}
	else
	{
		sysinfoMap.insert("机器类型", "未知机型");
	}
	int i = 0;
	QString collecttype;

	while (AppInfo::dev_cfg.portCollect[i] != 0)
	{
		collecttype.append(QString::number(AppInfo::dev_cfg.portCollect[i]));
		i++;
	}
	sysinfoMap.insert("采集顺序", collecttype);
	QString cycleorder;
	i=0;

	while (AppInfo::dev_cfg.CycleCollect[i] != 0)
	{
		cycleorder.append(QString::number(AppInfo::dev_cfg.CycleCollect[i]));
		i++;
	}
	// qDebug()<<"周期顺序"<<cycleorder;
	sysinfoMap.insert("周期顺序", cycleorder);
	sysinfoMap.insert("信号方式",
			AppInfo::sys_func_cfg.isIOCollect() ? "IO模块DI采集" : "采集器DI采集");

}

void MainConfigPage::switchInfoMapFun()
{
	switchMap.clear();
	if (AppInfo::GetInstance().isButtonBuzzer())
	{
		switchMap.insert("按键声音", "开");
	}
	else
	{
		switchMap.insert("按键声音", "关");
	}

	if (AppInfo::GetInstance().isUsbPower())
	{
		switchMap.insert("USB开关", "开");
	}
	else
	{
		switchMap.insert("USB开关", "关");
	}
}

void MainConfigPage::on_bt_voice_clicked()
{
	if (AppInfo::GetInstance().isButtonBuzzer())
	{
		AppInfo::GetInstance().setButtonBuzzer(false);
	}
	else
	{
		AppInfo::GetInstance().setButtonBuzzer(true);
	}
	AppInfo::GetInstance().saveConfig();

}
void MainConfigPage::on_bt_usb_clicked()
{
	if (AppInfo::GetInstance().isUsbPower())
	{
		AppInfo::GetInstance().setUsbPower(false);
		GPIO::getInstance().Write(PORT_USB_POWER, 0);
	}
	else
	{
		AppInfo::GetInstance().setUsbPower(true);
		GPIO::GPIO::getInstance().Write(PORT_USB_POWER, 1);
	}
	AppInfo::GetInstance().saveConfig();
}

void MainConfigPage::init_button_state()
{
	if (AppInfo::GetInstance().isButtonBuzzer())
	{
		ui->bt_voice->setChecked(true);
	}
	if (AppInfo::GetInstance().isUsbPower())
	{
		ui->bt_usb->setChecked(true);
	}
}
void MainConfigPage::dirfilelist()
{
	QString extension = "bin";
	QDir dir(dirpath);
	if (!dir.exists()) return;
	dir.setFilter(QDir::Dirs|QDir::Files);
	//dir.setSorting(QDir::DirsFirst);
	dir.setSorting(QDir::Time |QDir::Reversed);
	//排序方式 修改时间从小到大
	QFileInfoList list = dir.entryInfoList();
	int i=0,filecont=0;
	do
	{
		QFileInfo fileInfo = list.at(i);
		if(fileInfo.fileName() == "." || fileInfo.fileName()== "..")
		{
			qDebug()<< "filedir=" <<fileInfo.fileName();
			i++;
			continue;
		}
		bool bisDir=fileInfo.isDir();
		if(bisDir == false)
		{
			QString currentFileName=fileInfo.fileName();
				bool Reght=currentFileName.endsWith(extension, Qt::CaseInsensitive);
				if(Reght)
				{
					fileList <<currentFileName;
					qDebug()<<"filelist sort="<<dirpath+"/"+currentFileName;
					filecont++;
				}
		} i++;
	}while(i<list.size());
}

void MainConfigPage::getFileInfo()
{
	for(int i=0; i < fileList.size();i++)
	{
		QFile file(dirpath+"/"+fileList[i]);
		if(!file.open( QIODevice::ReadOnly | QIODevice::Text))
		{
			 qDebug()<<"Can't open the file!";
			 continue;
		}
		fileinfo tempfile;
		tempfile.filename = fileList[i];
		tempfile.filesize =QString::number(file.size());
		qDebug()<<tempfile.filename <<tempfile.filesize;
		QDataStream stream( &file );
		char ver[12];
		stream.readRawData(ver,6);
		tempfile.filever.sprintf("%02d-%02d-%02d-%02d-%02d-%02d",ver[0],ver[1],ver[2],ver[3],ver[4],ver[5]);
		qDebug()<<tempfile.filever;
		dirfile.append(tempfile);
		file.close();
	}
}

void MainConfigPage::showfile()
{
	QStringList filelist = AppInfo::getConfigNameList();
	for(int i=0; i<filelist.size(); i++)
	{
		int j;
		for(j=0;j <dirfile.size();j++)
		{
			if(filelist[i] == dirfile[j].filename)
			{
				ui->tw_file->setItem(i, 0, new QTableWidgetItem(dirfile[j].filename));//文件名
				ui->tw_file->setItem(i, 1, new QTableWidgetItem(dirfile[j].filever));//版本
				ui->tw_file->setItem(i, 2, new QTableWidgetItem(dirfile[j].filesize));//大小
				QTableWidgetItem  *item3 = new QTableWidgetItem("OK");
				item3->setIcon(QIcon(":/rc/res/right.png"));
				ui->tw_file->setItem(i, 3, item3);//状态
				break;
			}
		}
		if(j >= dirfile.size())
		{
			ui->tw_file->setItem(i, 0, new QTableWidgetItem(filelist[i]));//文件名
			QTableWidgetItem  *item4 = new QTableWidgetItem("FAILL");
			item4->setIcon(QIcon(":/rc/res/error.png"));
			item4->setBackgroundColor(QColor(200,255,200));
			ui->tw_file->setItem(i, 3, item4);    //状态
		}

	}

}

void MainConfigPage::handleTabPress(int index)
{

	//logDebug("current index:%d", index);
//	init_deviceInfo();
//	init_sysInfo();
//	switchInfoMapFun();
//	init_switchInfo();
	// QWidget *child = tabwidget->widget(index);
	if(index == 0)
	{
#ifndef _WIN32
	initNetMap("eth0");
	initNetMap("wlan0");
#endif
		sysInfoMapFun();
		switchInfoMapFun();
		init_button_state();
		init_deviceInfo();
		init_sysInfo();
		init_switchInfo();
	}
	else if(index ==2)
	{
		dirfilelist();
		getFileInfo();
		showfile();
	}

}

void MainConfigPage::on_bt_NetCommand_clicked()
{
    if(NetCommandService::GetInstance()->isStart())
        NetCommandService::GetInstance()->close();
    else
        NetCommandService::GetInstance()->start();
    ui->bt_NetCommand->setChecked(NetCommandService::GetInstance()->isStart());
}
