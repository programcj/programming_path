/*
 * ButtonStatus.cpp
 *
 *  Created on: 2015年5月20日
 *      Author: cj
 */

#include "ButtonStatus.h"

const char* ButtonStatus::getClassName() const
{
	return "ButtonStatus";
}

ButtonStatus::ButtonStatus(const QString &name, int index, int status)
{
	this->name = name;
	funIndex = index;
	showStatus = status;
	chanageTime = Tool::GetCurrentDateTimeStr();
}

MESPDFuncCfg::SetKeyInfo* ButtonStatus::getKeyInfo()
{
	return AppInfo::GetInstance().pd_func_cfg.getSetKeyInfo(funIndex);
}
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
UIPageButtonStatus *UIPageButtonStatus::instance;

UIPageButtonStatus *UIPageButtonStatus::GetInstance()
{
	return instance;
}

UIPageButtonStatus::UIPageButtonStatus(QObject *parent) :
		QObject(parent)
{
	instance = this;
	fileSavePath = AppInfo::GetInstance().getPathFile_FunButtonStatus();
}

UIPageButtonStatus::~UIPageButtonStatus()
{
	qDeleteAll(uiBtStatus);
	uiBtStatus.clear();
}

void UIPageButtonStatus::XMLFileReadUIBtStatus()
{
	qDeleteAll(uiBtStatus);
	uiBtStatus.clear();
	QFile file(fileSavePath);
	/* 如果功能按钮状态文件不存在时，就从功能配置中读取按钮配置到 uiBtStatus 中
	 * 如果存在，就从按钮状态文件中读取
	 */
	if (!QFile::exists(fileSavePath))
	{

		for (int i = 0;
				i < AppInfo::GetInstance().pd_func_cfg.SetKeyList.size(); i++)
		{
			uiBtStatus.append(
					new ButtonStatus(
							AppInfo::GetInstance().pd_func_cfg.SetKeyList[i].name,
							AppInfo::GetInstance().pd_func_cfg.SetKeyList[i].funIndex,
							1)); //默认为结束刷卡
		}
		QDomDocument doc;
		//PropertyListToXML(doc, uiBtStatus);
		QDomElement qlist = doc.createElement("QList");
		for (int i = 0; i < uiBtStatus.size(); i++)
		{
			qlist.appendChild(
					PropertyBaseToXML(*uiBtStatus[i]).documentElement());
		}
		doc.appendChild(qlist);
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
		QList<ButtonStatus*> uiBtStatusTmp; //UI按钮状态
		if (file.open(QIODevice::ReadOnly | QFile::Text))
		{
			//XMLFileToPropertList(file, uiBtStatusTmp);
			QDomDocument document;
			QString errorStr;
			int errorLine;
			int errorColumn;
			QTextStream floStream(&file);
			QTextCodec *codec = QTextCodec::codecForName("GB2312");
			floStream.setCodec(codec);
			QString xmlDataStr = floStream.readAll();

			if (document.setContent(xmlDataStr, false, &errorStr, &errorLine,
					&errorColumn))
			{
				QDomElement root = document.documentElement();
				QDomNodeList qlist = root.childNodes();
				for (int i = 0; i < qlist.count(); i++)
				{
					ButtonStatus *info = new ButtonStatus();
					XMLRead(qlist.item(i).toElement(), *info);
					uiBtStatusTmp.append(info);
				}
			}
			file.close();
		}

		int size = AppInfo::GetInstance().pd_func_cfg.SetKeyList.size();
		//
		qDeleteAll(uiBtStatus);
		uiBtStatus.clear();

		for (int i = 0; i < size; i++)
		{
			MESPDFuncCfg::SetKeyInfo keyInfo =
					AppInfo::GetInstance().pd_func_cfg.SetKeyList[i]; //默认为结束刷卡

			ButtonStatus *status = new ButtonStatus(keyInfo.name,
					keyInfo.funIndex, 1); //默认为结束刷卡
            //如果是帮定的内容呢？

			int tmpSize = uiBtStatusTmp.size();
			for (int j = 0; j < tmpSize; j++)
			{
				if (uiBtStatusTmp[j]->funIndex == status->funIndex)
				{
					status->name = keyInfo.name;

					if (uiBtStatusTmp[j]->isShowStart())
						status->setShowStartCard();
					else
						status->setShowEndCard();
                    status->setBindText(uiBtStatusTmp[j]->getBindText());
				}
			}
			uiBtStatus.append(status);
		}

		qDeleteAll(uiBtStatusTmp);
		uiBtStatusTmp.clear();
	}

	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		connect(uiBtStatus[i], SIGNAL(onStatChange(ButtonStatus*)), this,
				SLOT(onButtonStatChang(ButtonStatus*))); /*待人      */
	}
	onButtonStatChang(0);
}

void UIPageButtonStatus::XMLFileWriteUIBtStatus()
{
	QFile file(fileSavePath);
	QDomDocument doc;
	//PropertyListToXML(doc, uiBtStatus);
	QDomElement qlist = doc.createElement("QList");
	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		qlist.appendChild(PropertyBaseToXML(*uiBtStatus[i]).documentElement());
	}
	doc.appendChild(qlist);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&file);
		out << doc.toString();
		file.close();
	}
}

void UIPageButtonStatus::changeButtonStatus(ButtonStatus* btStatus)
{
	if (btStatus != 0)
	{
		if (btStatus->isShowStart())
			btStatus->setShowEndCard();
		else
			btStatus->setShowStartCard();
		XMLFileWriteUIBtStatus();
	}
}

ButtonStatus* UIPageButtonStatus::getUIBtStatus(
		MESPDFuncCfg::SetKeyInfo& keyInfo)
{
	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		if (uiBtStatus[i]->funIndex == keyInfo.funIndex)
		{
			return uiBtStatus[i];
		}
	}
	uiBtStatus.append(new ButtonStatus(keyInfo.name, keyInfo.funIndex, 0));
	return uiBtStatus[uiBtStatus.size() - 1];
}

ButtonStatus* UIPageButtonStatus::getUIBtStatus(int funIndex)
{
	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		if (uiBtStatus[i]->funIndex == funIndex)
		{
			return uiBtStatus[i];
		}
	}
	return 0;
}

bool UIPageButtonStatus::isStopCard()
{
	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		ButtonStatus *status = uiBtStatus[i];
		MESPDFuncCfg::SetKeyInfo* keyInfo = status->getKeyInfo();
		if (keyInfo != 0)
			if (keyInfo->isStopMachineFun())
				if (status->isShowStart())
				{
					return true;
				}
	}
	return false;
}

bool UIPageButtonStatus::isOtherStopCard(ButtonStatus* btStatus)
{
	for (int i = 0; i < uiBtStatus.size(); i++)
	{
		ButtonStatus *status = uiBtStatus[i];
		if (status->funIndex != btStatus->funIndex)
		{
			if (status->isShowStart())
			{   //显示状态为开始停机了
				MESPDFuncCfg::SetKeyInfo* keyInfo = status->getKeyInfo();
				if (keyInfo != 0)
				{
					if (keyInfo->isStopMachineFun())
					{ //如果是停机功能卡
						return true;
					}
				}
			}
		}
	}
	return false;
}

void UIPageButtonStatus::onButtonStatChang(ButtonStatus* bt)
{
	if (bt != 0)
		logDebug(
				QString("按钮改变:%1,%2,%3").arg(bt->name).arg(bt->funIndex).arg(
						bt->isShowEnd() ? 1 : 0));
	else
		logDebug(QString("按钮改变判断 %1").arg(isStopCard()));

	if (isStopCard())
		AppInfo::GetInstance().setHaveStopCard(true);
	else
		AppInfo::GetInstance().setHaveStopCard(false);
	AppInfo::GetInstance().saveConfig();

	if (bt != 0)
		XMLFileWriteUIBtStatus();

	emit onSigFunButtonChange(bt);
}
