/*
 * UIPageButton.cpp
 *
 *  Created on: 2015年2月5日
 *      Author: cj
 */

#include "UIPageButton.h"

void ButtonStatus::onBindProperty()
{
	addProperty("name", Property::AsQStr, &name);
	addProperty("funIndex", Property::AsInt, &funIndex);
	addProperty("showStatus", Property::AsInt, &showStatus);
	addProperty("chanageTime", Property::AsQStr, &chanageTime);
    addProperty("bindText", Property::AsQStr, &bindText);
}

ButtonStatus::ButtonStatus()
{
	funIndex = -1;
	showStatus = 1; //默认为结束刷卡
}

UIPageButton::UIPageButton()
{
	pageIndex = 0;
	pageSum = 0;
	this->getAllButtonStatus().XMLFileReadUIBtStatus();
	signalMapper = new QSignalMapper(this);

	connect(signalMapper, SIGNAL(mapped(QWidget *)), this,
			SLOT(onClick(QWidget *)));
}

void UIPageButton::show()
{
	//生产工单功能区按键配置文件 读取 可配置按键信息
	int btSumSize = AppInfo::GetInstance().pd_func_cfg.SetKeyList.size();
	int thePageSize = pageButtonList.size(); //本页个数

	for (int i = 0; i < thePageSize; i++)
	{
		int index = pageIndex * thePageSize + i; //当前的 哪一页 的按钮
		if (index < btSumSize)
		{ //
		  //配置按键信息
			MESPDFuncCfg::SetKeyInfo keyInfo =
					AppInfo::GetInstance().pd_func_cfg.SetKeyList[index];
			if (keyInfo.isStopMachineFun())
				pageButtonList[i]->setText("*" + keyInfo.name); //更改按钮的文本
			else
				pageButtonList[i]->setText(keyInfo.name); //更改按钮的文本

			//获取按钮状态
			ButtonStatus *btStatus = this->getAllButtonStatus().getUIBtStatus(
					keyInfo);
			if (btStatus != 0)
			{
				if (btStatus->isShowStart())
				{ //如果是开始停机卡
					pageButtonList[i]->setStyleSheet(
							"QPushButton {\
                                    border-radius: 0px; \
                                    color: rgb(255, 255, 0);\
                                    font: bold 16px;\
                                    border: 0px solid  rgb(29, 29, 29); \
                                    border-right: 2px solid  rgb(29, 29, 29);\
                                    border-bottom: 2px solid  rgb(29, 29, 29);\
                                    background-color: rgb(161, 161, 161); \
                            }");
				}
				else
				{
					//结束刷卡
					pageButtonList[i]->setStyleSheet(
							"QPushButton { \
                                    border: 0px solid  rgb(29, 29, 29); \
                                    border-radius: 0px;	\
                                    color: rgb(255, 255, 255);\
                                    font:bold 16px;\
                                    border-right: 4px solid  rgb(29, 29, 29);\
                                    border-bottom: 2px solid  rgb(29, 29, 29);\
                                    background-color: rgba(133, 172, 255, 150);  \
                            }");
				}
			}
		}
		else
		{
			pageButtonList[i]->setText("");
			pageButtonList[i]->setStyleSheet("");
		}
	}
}

//上一页
void UIPageButton::upPage()
{
	if (pageIndex > 0)
		pageIndex--;
	show();
}
//下一页
void UIPageButton::downPage()
{
	if (pageIndex < pageSum)
		pageIndex++;
	show();
}
//初始化
void UIPageButton::init()
{
	pageIndex = 0;
	pageSum = AppInfo::GetInstance().pd_func_cfg.SetKeyList.size()
			/ pageButtonList.size();
	logDebug(QString("总共页：%1").arg( pageSum));
	initButton();
	show();
}
//
void UIPageButton::initButton()
{
	for (int i = 0; i < pageButtonList.size(); i++)
	{
		pageButtonList[i]->disconnect();	//断开与某个对象相关联的任何对象。
		connect(pageButtonList[i], SIGNAL(clicked()), signalMapper,
				SLOT(map()));
		signalMapper->setMapping(pageButtonList[i], pageButtonList[i]);
	}
}
//按钮按下后的选择
void UIPageButton::onClick(QWidget* button)
{
	click((QPushButton*) button);
}

int UIPageButton::getButtonClickIndex(QPushButton* button)
{
	int index = -1;
	for (int i = 0; i < pageButtonList.size(); i++)
	{
		if (button == pageButtonList[i])
		{
			index = i;
			break;
		}
	}
	return index;
}

void UIPageButton::click(QPushButton* button)
{
	//找到这个按钮的信息：功能名称：功能序号 功能权限
	int index = getButtonClickIndex(button);
	if (index == -1)
	{
		return;
	}

	int btIndex = pageIndex * pageButtonList.size() + index; //哪一按钮
	if (btIndex >= AppInfo::GetInstance().pd_func_cfg.SetKeyList.size())
		return;
	MESPDFuncCfg::SetKeyInfo keyInfo =
			AppInfo::GetInstance().pd_func_cfg.SetKeyList[btIndex];
    //qDebug()<<QString("点击功能：%1, %2").arg(keyInfo.name).arg(keyInfo.funIndex)<<endl;
	switch (keyInfo.funIndex)
	{
	case 21:
		emit funButton21(keyInfo.funIndex);
		break;	//换模
	case 22:
		emit funButton22(keyInfo.funIndex);
		break;	//换料
	case 23:
		emit funButton23(keyInfo.funIndex);
		break;	//换单
	case 24:
		emit funButton24(keyInfo.funIndex);
		break;	//辅设故障
	case 25:
		emit funButton25(keyInfo.funIndex);
		break;	//机器故障
	case 26:
		emit funButton26(keyInfo.funIndex);
		break;	//模具故障
	case 27:
		emit funButton27(keyInfo.funIndex);
		break;	//待料
	case 28:
		emit funButton28(keyInfo.funIndex);
		break;	//保养
	case 29:
		emit funButton29(keyInfo.funIndex);
		break;	//待人
	case 30:
		emit funButton30(keyInfo.funIndex);
		break;	//交接班刷卡
	case 31:
		emit funButton31(keyInfo.funIndex);
		break;	//原材料不良
	case 32:
		emit funButton32(keyInfo.funIndex);
		break;	//计划停机
	case 33:
		emit funButton33(keyInfo.funIndex);
		break;	//上班
	case 34:
		emit funButton34(keyInfo.funIndex);
		break;	//下班
	case 35:
		emit funButton35(keyInfo.funIndex);
		break;	//修改周期
	case 36:
		emit funButton36(keyInfo.funIndex);
		break;	//调机
	case 37:
		emit funButton37(keyInfo.funIndex);
		break;	//调单
	case 38:
		emit funButton38(keyInfo.funIndex);
		break;	//调整模穴
	case 39:
		emit funButton39(keyInfo.funIndex);
		break;	//工程等待
	case 40:
		emit funButton40(keyInfo.funIndex);
		break;	//投料
	case 41:
		emit funButton41(keyInfo.funIndex);
		break;	//巡机
	case 42:
		emit funButton42(keyInfo.funIndex);
		break;	//打磨
	case 43:
		emit funButton43(keyInfo.funIndex);
		break;	//工单调拨
	case 44:
		emit funButton44(keyInfo.funIndex);
		break;	//试模
	case 45:
		emit funButton45(keyInfo.funIndex);
		break;	//设备点检
	case 46:
		emit funButton46(keyInfo.funIndex);
		break;	//耗电量
	case 47:
		emit funButton47(keyInfo.funIndex);
		break;	//试料
	}
}
