/*
 * Timer.h
 *
 *  Created on: 2019年10月25日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */

#ifndef SRC_APP_TIMER_H_
#define SRC_APP_TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

struct Timer
{
	struct Timer *prev;
	struct Timer *next;

	int interval; //间隔时间 ms
	uint64_t startTime; //开始时间
	void *user; //回调参数
	void (*func)(struct Timer *timer, void *user);
};

struct Timer *Timer_Init(struct Timer *timer, int interval, void *user,
		void (*func)(struct Timer *timer, void *user));

//启动定时器,可以重复启动,支持多线程
void Timer_Start(struct Timer *timer);

//停止定时器
void Timer_Stop(struct Timer *timer);

//调用定时器
void Timer_CallNext();

#ifdef __cplusplus
}
#endif

#endif /* SRC_APP_TIMER_H_ */
