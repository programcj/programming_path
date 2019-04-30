/*
 * app_thread.h
 *
 *  Created on: 2017年12月21日
 *      Author: cj
 */

#ifndef _APP_THREAD_H_
#define _APP_THREAD_H_

#include <pthread.h>

#define OS_THREAD_CONF_NUMEVENTS      32

typedef struct app_thread_event {
	uint8_t ev;
	void *data;
} app_thread_event_t;

/**
 * _ 开头代表私有
 */
typedef struct app_thread {
	const char *name;
	void *(*_run)(struct app_thread *thread, void *context);
	app_thread_event_t events[OS_THREAD_CONF_NUMEVENTS];
	uint8_t _events_pos;
	uint8_t _events_length;
	void *context;

	uint8_t _interruption_requested;
	uint8_t _running;
	pthread_t _pt;
	pthread_mutex_t _lock;
	pthread_rwlock_t _lock_events;
//async handle
//
} app_thread_t;

int app_thread_exec(app_thread_t *thread, void *context);

int app_thread_kill(app_thread_t *thread, int sig);

int app_thread_interrupted(app_thread_t *thread);

int app_thread_isinterrupted(app_thread_t *thread);

int app_thread_is_running(app_thread_t *thread);

int app_thread_event_post(app_thread_t *thread, uint8_t ev, void *data);

int app_thread_event_pop(app_thread_t *thread, uint8_t *ev, void **data);

#define APP_THREAD(thread_name)   app_thread_t  thread_name = { .name=#thread_name, thread_name##_run, \
		._events_pos = 0, ._events_length = 0, \
		._interruption_requested=0, \
		._pt=0, \
		._lock=PTHREAD_MUTEX_INITIALIZER, \
		._lock_events = PTHREAD_RWLOCK_INITIALIZER }

#define APP_THREAD_RUN(thread_name)	void *thread_name##_run(struct app_thread *thread, void *context)

#endif /* _APP_THREAD_H_ */
