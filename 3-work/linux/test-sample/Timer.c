/*
 * Timer.c
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
#include <pthread.h>
#include "Timer.h"

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

struct TimerList
{
	struct Timer *prev;
	struct Timer *next;
};

static struct TimerList _list =
{ .prev = (struct Timer*) &_list, .next = (struct Timer*) &_list };

static uint64_t os_curr_timems()
{
	struct timespec times =
	{ 0, 0 };
	uint64_t time;
	clock_gettime(CLOCK_MONOTONIC, &times);
	time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	return time;
}

struct Timer *Timer_Init(struct Timer *timer, int interval, void *user,
		void (*func)(struct Timer *timer, void *user))
{
	timer->startTime = os_curr_timems();
	timer->interval = interval;
	timer->func = func;
	return timer;
}

void Timer_Start(struct Timer *timer)
{
	struct Timer *n, *p;
	int exists = 0;
	pthread_mutex_lock(&_mutex);
	for (p = _list.next, n = p->next; p != (struct Timer*) &_list;
			p = n, n = n->next)
	{
		if (p == timer)
		{
			exists = 1;
			timer->startTime = os_curr_timems();
			break;
		}
	}
	if (!exists)
	{
		timer->startTime = os_curr_timems();
		_list.prev->next = timer;
		timer->prev = _list.prev;
		timer->next = (struct Timer*)&_list;
		_list.prev = timer;
	}
	pthread_mutex_unlock(&_mutex);
}

void Timer_Stop(struct Timer *timer)
{
	struct Timer *n, *p;
	pthread_mutex_lock(&_mutex);
	for (p = _list.next, n = p->next; p != (struct Timer*) &_list;
			p = n, n = n->next)
	{
		if (p == timer)
		{
			p->prev->next = p->next;
			p->next->prev = p->prev;
			break;
		}
	}
	pthread_mutex_unlock(&_mutex);
}

static struct Timer *Timer_Pop()
{
	struct Timer *n, *p;
	struct Timer *timer = NULL;
	uint64_t ostime = os_curr_timems();
	pthread_mutex_lock(&_mutex);
	for (p = _list.next, n = p->next; p != (struct Timer*) &_list;
			p = n, n = n->next)
	{
		if (ostime - p->startTime > p->interval)
		{
			p->prev->next = p->next;
			p->next->prev = p->prev;
			timer = p;
			break;
		}
	}
	pthread_mutex_unlock(&_mutex);
	return timer;
}

void Timer_CallNext()
{
	struct Timer *timer = NULL;
	timer = Timer_Pop();
	if (timer)
		timer->func(timer, timer->user);
}
