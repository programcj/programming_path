#include "toolicscandialog.h"
#include "tooltextinputdialog.h"
#include "plugin/mesmessagedlg.h"

ToolICScanDialog::ToolICScanDialog(QWidget *parent) :
		QDialog(parent), ui(new Ui::ToolICScanDialog)
{
	ui->setupUi(this);
//	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); //无标题
//	setGeometry(0, 0, 800, 480);
//	setAttribute(Qt::WA_TranslucentBackground); //透明处理
}

ToolICScanDialog::ToolICScanDialog(const MESPDFuncCfg::SetKeyInfo* keyInfo,
		QWidget* parent) :
		QDialog(parent), ui(new Ui::ToolICScanDialog)
{
	ui->setupUi(this);
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); //无标题
	setGeometry(0, 0, 800, 480);
	setAttribute(Qt::WA_TranslucentBackground); //透明处理

	if (keyInfo != 0)
	{
		m_keyInfo.funIndex = keyInfo->funIndex;
		m_keyInfo.funP = keyInfo->funP;
		m_keyInfo.name = keyInfo->name;
		m_keyInfo.status = keyInfo->status;
	}
	RFIDServer::GetInstance()->RFIDStart();
	connect(RFIDServer::GetInstance(), SIGNAL(RFIDData(char*)), this,
			SLOT(RFIDData(char*)));
}

ToolICScanDialog::~ToolICScanDialog()
{
	RFIDServer::GetInstance()->RFIDEnd();
	delete ui;
}

void ToolICScanDialog::on_pushButton_clicked()
{
	ToolTextInputDialog dialog(NULL,ToolTextInputDialog::typeText,"","请输入刷卡密码");
	if (dialog.exec() == ToolTextInputDialog::KeyEnter)
	{
		icCardNo = "00000001";
		if (dialog.text() != AppInfo::GetInstance().getBrushCardPassword())
		{
			ToastDialog::Toast(NULL, "密码出错!", 2000);
			close();
			return;
		}
		AppInfo::GetInstance().setBrushCardId(icCardNo);
		AppInfo::GetInstance().saveConfig();
		accept();
	}
}

const QString ToolICScanDialog::getICCardNo()
{
	return icCardNo;
}

void ToolICScanDialog::on_pushButton_2_clicked()
{
	icCardNo = "931FC8E2";
	//这里须要从数据库中寻找
	Employee employee;
	if (!Employee::queryIDCardNo(employee, icCardNo))
	{
		ToastDialog::Toast(NULL, "用户不存在", 2000);
		return;
	}

	//还有权限判断
	//原理说明:从chiefmes 第二版本而来
	//HEX: A1 23 45 67  >>0xA1234567   [0]=67
	//OBJ: 67 45 A1 23  >>0x67452123   [0]=23
	//			ucCarPrivilege[0] = Asii_To_Hex(g_TempMng.cPrivilege[0]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[1]);
	//			ucCarPrivilege[1] = Asii_To_Hex(g_TempMng.cPrivilege[2]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[3]);
	//			ucCarPrivilege[2] = Asii_To_Hex(g_TempMng.cPrivilege[4]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[5]);
	//			ucCarPrivilege[3] = Asii_To_Hex(g_TempMng.cPrivilege[6]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[7]);
	//
	//			g_udwGetCardPrivileges = (ucCarPrivilege[1])|(ucCarPrivilege[0] << 8)|(ucCarPrivilege[3] << 16)|(ucCarPrivilege[2] << 24);
	//
	//			if ( ((g_udwGetCardPrivileges&g_udwNeedCardPrivileges) != 0)
	//				&& (g_udwGetCardPrivileges >= g_udwNeedCardPrivileges) )
	//			{
	//				bIsHavePrivileges = TRUE;
	//			}
	//			else
	//			{
	//				bIsHavePrivileges = FALSE;
	//			}
	{
		QString IcCardRight = employee.getIcCardRight();
		unsigned long privilege = 0;

		sscanf(IcCardRight.toAscii().data(), "%x", &privilege);

		unsigned short v = 0x0000FFFF & privilege;
		v = (v & 0x00FF) << 8 | (v & 0xFF00) >> 8;
		privilege = (privilege & 0xFFFF00) | v;
		privilege = (privilege & 0x0000FFFF) << 16
				| (privilege & 0xFFFF0000) >> 16;

		if (privilege & m_keyInfo.funP != 0)
		{
			AppInfo::GetInstance().setBrushCardId(icCardNo);
			AppInfo::GetInstance().saveConfig();
			accept();
		}
		else
		{
			ToastDialog::Toast(NULL, "没有权限", 2000);
		}
	}
}

void ToolICScanDialog::RFIDData(char* icBuff)
{
	QByteArray arry;
	arry.append(icBuff, 8);
	icCardNo.prepend(arry);
	qDebug() << icCardNo;
	//这里须要从数据库中寻找
	Employee employee;
	if (!Employee::queryIDCardNo(employee, icCardNo))
	{
		ToastDialog::Toast(NULL, "用户不存在", 2000);
		return;
	}

	//还有权限判断
	//原理说明:从chiefmes 第二版本而来
	//HEX: A1 23 45 67  >>0xA1234567   [0]=67
	//OBJ: 67 45 A1 23  >>0x67452123   [0]=23
	//			ucCarPrivilege[0] = Asii_To_Hex(g_TempMng.cPrivilege[0]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[1]);
	//			ucCarPrivilege[1] = Asii_To_Hex(g_TempMng.cPrivilege[2]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[3]);
	//			ucCarPrivilege[2] = Asii_To_Hex(g_TempMng.cPrivilege[4]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[5]);
	//			ucCarPrivilege[3] = Asii_To_Hex(g_TempMng.cPrivilege[6]) <<4 |  Asii_To_Hex(g_TempMng.cPrivilege[7]);
	//
	//			g_udwGetCardPrivileges = (ucCarPrivilege[1])|(ucCarPrivilege[0] << 8)|(ucCarPrivilege[3] << 16)|(ucCarPrivilege[2] << 24);
	//
	//			if ( ((g_udwGetCardPrivileges&g_udwNeedCardPrivileges) != 0)
	//				&& (g_udwGetCardPrivileges >= g_udwNeedCardPrivileges) )
	//			{
	//				bIsHavePrivileges = TRUE;
	//			}
	//			else
	//			{
	//				bIsHavePrivileges = FALSE;
	//			}
	{
		QString IcCardRight = employee.getIcCardRight();
		unsigned long privilege = 0;

		sscanf(IcCardRight.toAscii().data(), "%x", &privilege);

		unsigned short v = 0x0000FFFF & privilege;
		v = (v & 0x00FF) << 8 | (v & 0xFF00) >> 8;
		privilege = (privilege & 0xFFFF00) | v;
		privilege = (privilege & 0x0000FFFF) << 16
				| (privilege & 0xFFFF0000) >> 16;

		if (privilege & m_keyInfo.funP != 0)
		{
			AppInfo::GetInstance().setBrushCardId(icCardNo);
			AppInfo::GetInstance().saveConfig();
			accept();
		}
		else
		{
			ToastDialog::Toast(NULL, "没有权限", 2000);
		}
	}
}

void ToolICScanDialog::on_pushButton_3_clicked()
{
	close();
}
