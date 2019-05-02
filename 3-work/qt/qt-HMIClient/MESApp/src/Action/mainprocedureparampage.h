#ifndef MAINPROCEDUREPARAMPAGE_H
#define MAINPROCEDUREPARAMPAGE_H

#include <QWidget>
#include "../Public/public.h"
#include "../Public/temperatureitem.h"

namespace Ui {
class MainProcedureParamPage;
}

class MainProcedureParamPage: public QWidget {
Q_OBJECT

public:
	explicit MainProcedureParamPage(QWidget *parent = 0);
	~MainProcedureParamPage();

	void Loading();

private slots:
	//模次信息增加
	void OnModeNumberAdd(int number, //次数
			const QString start, //开始时间
			const QString endTime, //结束时间
			unsigned long machineCycle, //机器周期，毫秒
			unsigned long FillTime, //填充时间，毫秒
			unsigned long CycleTime //成型周期
			);

	void OnMESConfigRefurbish(const QString &str);

private:
	virtual void timerEvent(QTimerEvent *e);
	QGraphicsScene* canvas;
	QList<TemperatureItem*> itemList;

	Ui::MainProcedureParamPage *ui;

};

#endif // MAINPROCEDUREPARAMPAGE_H
