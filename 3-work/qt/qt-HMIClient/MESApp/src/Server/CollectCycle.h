/*
 * CollectCycle.h
 *
 *  Created on: 2015年3月19日
 *      Author: cj
 */

#ifndef COLLECTCYCLE_H_
#define COLLECTCYCLE_H_

#include <QThread>
#include "../Public/cycleData.h"
#include "../Public/public.h"
/*
 * 数据采集功能的
 */
class CollectCycle: public QThread
{
Q_OBJECT

public:
	bool isInterrupted();
	void interrupt();

	static CollectCycle *GetInstance();
	CycleData collectCycleData;
	 bool Dev_io_get_input(const quint8 portNo, quint16 &value);
	 bool Rs485_collect_read(const quint8 portNo, quint16 &value);
	//void collect_module_data();

private:
	Producted producted;
    Order order;
	//static CollectCycle instance;
	bool isInterruptedFlag;
	virtual void run();
};

#endif /* COLLECTCYCLE_H_ */
