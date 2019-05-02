#ifndef MAINPRODUCTORDERPAGE_H
#define MAINPRODUCTORDERPAGE_H

#include <QtGui>
#include <QWidget>
#include "../Public/public.h"
#include "UIPageButton.h"
#include "fun_dialog/fun24fsgz.h"
#include "fun_dialog/fun36tj.h"
#include "fun_dialog/fun37td.h"
#include "fun_dialog/fun38adjsocketnum.h"
#include "fun_dialog/fun40tl.h"
#include "fun_dialog/fun41xj.h"
#include "fun_dialog/fun42dm.h"
#include "fun_dialog/fun45sbdj.h"
#include "fun_dialog/fun48pzlr.h"
#include "fun_dialog/fun49cpgl.h"
#include "fun_dialog/fun43ordermove.h"

namespace Ui {
class MainProductOrderPage;
}
/**
 * 生产工单界面
 *   这里主要是功能按钮显示：
 *         1. 依据多停机卡  判断是否有其它停机卡
 *         2. 按下前 显示为结束状态 改变为 开始状态
 *         3. 按下前 显示为开始状态 改变为 结束状态
 * a: cj
 */
class MainProductOrderPage: public QWidget {
Q_OBJECT

public:
	explicit MainProductOrderPage(QWidget *parent = 0);
	~MainProductOrderPage();

	OrderIndex orderIndex;
	void showOrder();

private slots:
	/***********************************************/
	bool OtherStopCardCheck(ButtonStatus *btStatus);
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

	////////////////////////////////////////////////////////////////
    void on_btUpPage_clicked(); //功能按钮 上一页
    void on_btNextPage_clicked(); //功能按钮　下一页

    void on_btUpItem_clicked(); //上件
    void on_btNextItem_clicked();//下件
    void on_btUpOrder_clicked();//上单
    void on_btNextOrder_clicked();//下单

	void OnOrderTableChange();	//工单表 有数据添加
    void OnConfigRefurbish(const QString &name); //配置文件改变

private:
	UIPageButton uiPageButton;
	Ui::MainProductOrderPage *ui;
};

#endif // MAINPRODUCTORDERPAGE_H
