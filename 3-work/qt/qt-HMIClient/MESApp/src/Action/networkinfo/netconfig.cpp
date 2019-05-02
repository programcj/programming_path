#include "netconfig.h"
#include <QFile>
#include "../../Public/public.h"
#include <QTextStream>
#include <QCloseEvent>
#include <QMessageBox>

netconfig::netconfig(QWidget *parent) :
		QDialog(parent)
{
	ui.setupUi(this);
	proc = NULL;
	flag = false;
	setWindowFlags(Qt::FramelessWindowHint);
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground);

	connect(ui.cb_interface, SIGNAL(currentIndexChanged(const QString &)), this,
			SLOT(on_sel_changed(QString)));
	connect(ui.bt_ok, SIGNAL(clicked()), this, SLOT(on_bt_ok_clicked()));
	connect(ui.bt_Exit, SIGNAL(clicked()), this, SLOT(reject()));
	//connect(ui.radio_dhcp, SIGNAL(toggled(bool)), this, SLOT(on_toggled(bool)));
	//connect(ui.radio_static, SIGNAL(toggled(bool)), this, SLOT(on_toggled(bool)));
#ifndef _WIN32
	refreshInterfaces();
	readConfigs();
	qDebug()<<AppInfo::GetInstance().getNetDeviceName();
	on_sel_changed(AppInfo::GetInstance().getNetDeviceName());
#endif
}

netconfig::~netconfig()
{
	// delete ui;
	foreach(Interface *i,ints)
		delete i;
	if (proc)
		delete proc;
}

void netconfig::state(bool dhcp)
{

}

void netconfig::refreshInterfaces()
{
	QStringList sl;

	/*æ©å›¨æŠ¤ç’‡ç±©th[0-9] wlan[0-9]*/
	::system(
			"ifconfig -a|grep -E \"eth[0-9]|wlan[0-9]\" | cut -d' ' -f 1 > /tmp/interfaces");
	QFile f("/tmp/interfaces");
	if (f.open(QFile::ReadOnly))
	{
		QTextStream t(&f);
		while (!t.atEnd())
		{
			QString str = t.readLine();
			if (str.size() > 0)
			{
				//QMessageBox::about(this,"aaa",str);
				ints.append(new Interface(str));
				sl << str;
			}
		}
	}
	f.close();

	ui.cb_interface->addItems(sl);
}

void netconfig::readConfigs()
{
	foreach(Interface *i,ints)
	{
		QFile f("/etc/network/.conf/" + i->name);
		if (f.open(QFile::ReadOnly))
		{
			QTextStream t(&f);
			QString str = t.readLine();
			if (str == "static")
			{
				// i->dhcp = false;

				i->ip = t.readLine();
				i->mask = t.readLine();
				i->gateway = t.readLine();
				i->serverip = t.readLine();
				i->portA = t.readLine().toInt();
				i->portB = t.readLine().toInt();
				//i->dns = t.readLine();
			}
		}
		f.close();
	}
}

unsigned int ip2int(QString ip)
{
	QStringList sl = ip.split('.');
	unsigned int r = 0;
	foreach(QString s,sl)
	{
		r <<= 8;
		r |= s.toUInt();
	}

	return r;
}

QString int2ip(unsigned int ip)
{
	return QString::number((ip >> 24) & 0xff) + "."
			+ QString::number((ip >> 16) & 0xff) + "."
			+ QString::number((ip >> 8) & 0xff) + "."
			+ QString::number(ip & 0xff);
}

void netconfig::writeConfigs()
{
	Interface *i = NULL;
	QFile m("/etc/network/interfaces");	//for interface
	QTextStream *t3 = NULL;
	if (m.open(QIODevice::WriteOnly | QFile::Truncate))
	{
		t3 = new QTextStream(&m);
	}
	*t3 << QString("auto lo\n");
	*t3 << QString("iface lo inet loopback\n");

	::system("mkdir -p /etc/network/.conf");

	foreach(i,ints)
	{
		if (i->name == ui.cb_interface->currentText())
			break;
	}

	QFile f("/etc/network/.conf/" + i->name);

	if (f.open(QFile::WriteOnly))
	{
		QTextStream t(&f);
		{

			t << QString("static") << QString("\n");
			t << i->ip << QString("\n");
			t << i->mask << QString("\n");
			t << i->gateway << QString("\n");
			t << i->serverip << QString(QString("\n"));
			t << QString::number(i->portA) << QString(QString("\n"));
			t << QString::number(i->portB) << QString(QString("\n"));
			// t<<i->dns<<QString("\n");

			*t3 << QString("auto ") << i->name << QString("\n");
			*t3 << QString("iface ") << i->name << QString(" inet static\n");
			*t3 << QString("address ") << i->ip << QString("\n");
			*t3 << QString("netmask ") << i->mask << QString("\n");
			*t3 << QString("gateway ") << i->gateway << QString("\n");
			*t3 << QString("broadcast ")
					<< int2ip(
							(ip2int(i->ip) & ip2int(i->mask))
									| (~ip2int(i->mask))) << QString("\n");

			//  *t2<<QString("nameserver ")+i->dns<<QString("\n");//éšå±¾æ¤‚éæ¬å†é’ï¿½etc/resolv.conf
		}
	}
	f.close();

	//delete t2;
	delete t3;

	// r.close();
	m.close();
}

void netconfig::on_sel_changed(const QString &text)
{
	Interface *i = NULL;
	foreach(i,ints)
	{
		if (i->name == text)
		{
			//qDebug()<<"on_sel_changedi"<<i->name;
			ui.cb_interface->setCurrentIndex(ui.cb_interface->findText(text,Qt::MatchCaseSensitive));
			break;
		}

	}

	{
		ui.edt_ip->setText(i->ip);
		ui.edt_mask->setText(i->mask);
		ui.edt_gateway->setText(i->gateway);
		// ui.edt_dns->setText(i->dns);
		ui.server_ip->setText(i->serverip);
		ui.port1->setText(QString::number(i->portA));
		ui.port2->setText(QString::number(i->portB));
	}
}
bool isIp(QString ip)
{
	QStringList iplist;
	iplist = ip.split('.');
	if (iplist.size() < 4)
	{
		return false;
	}

	if (iplist[0].trimmed().toInt() > 255 || iplist[1].trimmed().toInt() > 255
			|| iplist[2].trimmed().toInt() > 255
			|| iplist[3].trimmed().toInt() > 255)
	{
		return false;
	}

	return true;
}
void netconfig::on_bt_ok_clicked()
{

	if (flag == true)
	{
		return;
	}

#ifndef _WIN32
	Interface *i = NULL;
	foreach(i,ints)
	{
		if (i->name == ui.cb_interface->currentText())
			break;
	}

	//  i->dhcp = ui.radio_dhcp->isChecked();
	i->ip = ui.edt_ip->text();
	qDebug() << "ip" << i->ip;
	i->mask = ui.edt_mask->text();
	qDebug() << "mask" << i->mask;
	i->gateway = ui.edt_gateway->text();
	qDebug() << "gateway" << i->gateway;
	i->serverip = ui.server_ip->text();

	i->portA = ui.port1->text().trimmed().toInt();
	i->portB = ui.port2->text().trimmed().toInt();

	if (!isIp(i->ip) || !isIp(i->mask) || !isIp(i->gateway)
			|| !isIp(i->serverip))
	{
		//QMessageBox::information(this, "", "ipåœ°å€é”™è¯¯");
		MESMessageDlg::about(this, "´íÎó", "ÊäÈëIPÓÐ´í");
		return;
	}
	writeConfigs();
#endif
	if (proc)
		delete proc;

	proc = new QProcess(this);
	this->setDisabled(true);
	flag = true;
#ifndef _WIN32
	proc->start("/etc/init.d/networking restart");
#else
	proc->start("ping 127.0.0.1");
#endif
	connect(proc, SIGNAL(finished(int)), this, SLOT(proc_finished(int)));

}

void netconfig::closeEvent(QCloseEvent * evt)
{
	if (flag)
	{
		evt->ignore();
		QMessageBox::about(this, "info", "network is restarting,please wait");
	}
	else
	{
		destroy();
	}
}

void netconfig::moveEvent(QMoveEvent *)
{
	this->move(QPoint(0, 0));
}

void netconfig::resizeEvent(QResizeEvent *)
{
	this->showMaximized();
}

void netconfig::proc_finished(int code)
{

	if (proc->exitStatus() == QProcess::NormalExit)
	{
		this->setDisabled(false);
		flag = false;
		//QMessageBox::about(this, "info", "network restart ok!");
		MESMessageDlg::about(this, "ÐÅÏ¢", "ÍøÂçÅäÖÃ³É¹¦£¡");
		disconnect(proc, SIGNAL(finished(int)), this, SLOT(proc_finished(int)));
		AppInfo::GetInstance().setDevIp(ui.edt_ip->text());
		AppInfo::GetInstance().setNetDeviceName(ui.cb_interface->currentText());
		AppInfo::GetInstance().OnUpdateNet(ui.server_ip->text(),
				ui.port1->text().trimmed().toInt(),
				ui.port2->text().trimmed().toInt());

	}

}

