/*
 * DigitalMeterDl64597.h
 *
 *  Created on: 2015年6月2日
 *      Author: cj
 */

#ifndef DIGITALMETERDL64597_H_
#define DIGITALMETERDL64597_H_

#include "../Public/public.h"
#include "serial/serial.h"

class DigitalMeter_Dl64597
{
public:
	DigitalMeter_Dl64597();
	virtual ~DigitalMeter_Dl64597();

	/**************************************************************
	 * 功能：发送读正向有功电能总量数据帧
	 * 出口：TRUE 发送成功，FALSE 发送失败
	 * 入口：无
	 * 说明：无
	 **************************************************************/
	bool DigitalMeter_send_read_ele_frame();

	// 数据读
	int DigitalMeter_read_data(quint32 *data);

	// 读电表数据
	bool getReadMeterFlag();

	// 设置电表数据
   static void PostElectMeter();

	void setSerialHandle( Serial* fd);

	bool saveMasterData(quint32 elecnum);



private:
	//计算电表数
	quint32 DigitalMeter_calc_data();

	// 计算效验码
	quint8 DTS72_check_number();

    static bool ReadFlag;    //true 读电表数， false 不读
	Serial* Masterserial;


};

#endif /* DIGITALMETERDL64597_H_ */
