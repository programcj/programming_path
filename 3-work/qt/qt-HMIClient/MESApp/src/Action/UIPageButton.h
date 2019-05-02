/*
 * UIPageButton.h
 *
 *  Created on: 2015年2月5日
 *      Author: cj
 */

#ifndef UIPAGEBUTTON_H_
#define UIPAGEBUTTON_H_

#include "../Public/public.h"
#include "../Public/ButtonStatus.h"
#include <QPushButton>

/*
 * //页按钮类
 */
class UIPageButton: public QObject
{
Q_OBJECT

	QList<QPushButton *> pageButtonList; //一页的按钮
	int pageIndex; //当前页
	int pageSum; //总共页

	QSignalMapper *signalMapper;
	//这一页的哪一按钮按下了
	void click(QPushButton *button);

	//获取哪一个按钮
	int getButtonClickIndex(QPushButton *button);
public:
	UIPageButton();

	//取得所有的按钮
	UIPageButtonStatus &getAllButtonStatus()
	{
        return *UIPageButtonStatus::GetInstance();
	}
	//添加按钮
	void appendButton(QPushButton *button)
	{
		pageButtonList.append(button);
	}
	//初始化
	void init();
	//初始化按钮事件
	void initButton();
	//显示工能按钮面
	void show();
	//上一页
	void upPage();
	//下一页
	void downPage();

signals:
	void funButton21(int funIndex); //	换模
	void funButton22(int funIndex); //	换料
	void funButton23(int funIndex); //	换单
	void funButton24(int funIndex); //	辅设故障
	void funButton25(int funIndex); //	机器故障
	void funButton26(int funIndex); //	模具故障
	void funButton27(int funIndex); //	待料
	void funButton28(int funIndex); //	保养
	void funButton29(int funIndex); //	待人
	void funButton30(int funIndex); //	交接班刷卡
	void funButton31(int funIndex); //	原材料不良
	void funButton32(int funIndex); //	计划停机
	void funButton33(int funIndex); //	上班
	void funButton34(int funIndex); //	下班
	void funButton35(int funIndex); //	修改周期
	void funButton36(int funIndex); //	调机
	void funButton37(int funIndex); //	调单
	void funButton38(int funIndex); //	调整模穴
	void funButton39(int funIndex); //	工程等待
	void funButton40(int funIndex); //	投料
	void funButton41(int funIndex); //	巡机
	void funButton42(int funIndex); //	打磨
	void funButton43(int funIndex); //	工单调拨
	void funButton44(int funIndex); //	试模
	void funButton45(int funIndex); //	设备点检
	void funButton46(int funIndex); //	耗电量
	void funButton47(int funIndex); //	试料

private slots:
	void onClick(QWidget *button);
};

#endif /* UIPAGEBUTTON_H_ */
