/*
 * iomodule.h
 *
 *  Created on: 2015年4月29日
 *      Author: cj
 */

#ifndef IOMODULE_H_
#define IOMODULE_H_

//功能码
#define FN_ART_READ_KEEP_CHANNEL			0x03    //标准读功能寄存器
#define FN_ART_READ_INPUT_CHANNEL			0x04   // 读输入寄存器    //  功能码


//****************** MODBUS 供能码 ***********************//
#define FN_READ_CHANNEL						FN_ART_READ_INPUT_CHANNEL
#define FN_WRITE_REG						0x10
//宏定义
#define THREAD_COLLECTED_ENABLE			 	1		// 1 = 使能线程采集		0 = 关闭
#define TEMPERATURE_SAVED_MAX				25
#define USE_ELE_CURRENT					0		// 1 = 压力电流     0 = 压力电压
//各模块地址
#define IO_DEVICE_ADDR_KND					1  // 康耐德 IO
#define IO_DEVICE_ADDR_ART_3038					2  // ART 3038
#define IO_DEVICE_ADDR_ART_3054					3  // ART 3054

//输入寄存器起始地址
#define IO_MODULE_START_ADDR_KND			0x0200
#define IO_MODULE_START_ADDR_ART			257   //0x7631 //40288// 十进制 30257


#define IO_REG_ADDR_CAB_TEMPER_0		0x020C
#define IO_REG_ADDR_CAB_TEMPER_1		0x020D
#define IO_REG_ADDR_CAB_TEMPER_2		0x020E
#define IO_REG_ADDR_CAB_TEMPER_3		0x020F

#endif /* IOMODULE_H_ */
