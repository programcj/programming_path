/*
 * app_thread.c
 *
 *  Created on: 2017年12月21日
 *      Author: cj
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <signal.h>
#include "app_thread.h"

static void _app_thread_signal_handle(int v) {

}

static void *_app_thread_run(void *arg) {
	app_thread_t *thread = (app_thread_t *) arg;
	prctl(PR_SET_NAME, thread->name);
	pthread_detach(pthread_self());
	signal(SIGINT, _app_thread_signal_handle);

	pthread_mutex_lock(&thread->_lock);
	thread->_running = 1;
	pthread_mutex_unlock(&thread->_lock);

	thread->_run(thread, thread->context);

	pthread_mutex_lock(&thread->_lock);
	thread->_running = 0;
	pthread_mutex_unlock(&thread->_lock);
	return NULL;
}

int app_thread_exec(app_thread_t *thread, void *context) {
	int ret = 0;
	thread->context = context;
	ret = pthread_create(&thread->_pt, NULL, _app_thread_run, thread);
	return ret;
}

int app_thread_kill(app_thread_t *thread, int sig) {
	int ret = 0;
	pthread_mutex_lock(&thread->_lock);
	if (thread->_running)
		ret = pthread_kill(thread->_pt, sig);
	else
		ret = -1;
	pthread_mutex_unlock(&thread->_lock);
	return ret;
}

int app_thread_interrupted(app_thread_t *thread) {
	int flag = 0;
	pthread_mutex_lock(&thread->_lock);
	thread->_interruption_requested = 1;
	pthread_mutex_unlock(&thread->_lock);
	app_thread_kill(thread, SIGINT);
	return flag;
}

int app_thread_isinterrupted(app_thread_t *thread) {
	int flag = 0;
	pthread_mutex_lock(&thread->_lock);
	flag = thread->_interruption_requested;
	pthread_mutex_unlock(&thread->_lock);
	return flag;
}

int app_thread_is_running(app_thread_t *thread) {
	int ret = 0;
	pthread_mutex_lock(&thread->_lock);
	ret = thread->_running;
	pthread_mutex_unlock(&thread->_lock);
	return ret;
}

int app_thread_event_post(app_thread_t *thread, uint8_t ev, void *data) {
	int rc = 0;
	int pos = 0;
	pthread_rwlock_wrlock(&thread->_lock_events);
#if 0
	if (((thread->_events_length + 1) % OS_THREAD_CONF_NUMEVENTS)
			!= thread->_events_pos) {
		thread->events[thread->_events_length].ev = ev;
		thread->events[thread->_events_length].data = data;
		thread->_events_length = (thread->_events_length + 1)
		% OS_THREAD_CONF_NUMEVENTS;
		app_thread_kill(thread, SIGINT);
		rc = 0;
	} else {
		rc = -1;
	}
#else
	if (thread->_events_length <= OS_THREAD_CONF_NUMEVENTS) {
		pos = (thread->_events_length + thread->_events_pos)
				% OS_THREAD_CONF_NUMEVENTS;
		thread->events[pos].ev = ev;
		thread->events[pos].data = data;
		thread->_events_length++;
		app_thread_kill(thread, SIGINT);
	}
#endif
	pthread_rwlock_unlock(&thread->_lock_events);
	return rc;
}

int app_thread_event_pop(app_thread_t *thread, uint8_t *ev, void **data) {
	int ret = 0;
	pthread_rwlock_wrlock(&thread->_lock_events);
#if 0
	if (thread->_events_pos != thread->_events_length) {
		*ev = thread->events[thread->_events_pos].ev;
		*data = thread->events[thread->_events_pos].data;
		thread->_events_pos = (thread->_events_pos + 1)
		% OS_THREAD_CONF_NUMEVENTS;
	} else {
		ret = -1;
	}
#else
	if (thread->_events_length > 0) {
		*ev = thread->events[thread->_events_pos].ev;
		if (data)
			*data = thread->events[thread->_events_pos].data;
		thread->_events_pos =
				(thread->_events_pos + 1) % OS_THREAD_CONF_NUMEVENTS;
		thread->_events_length--;
	} else {
		ret = -1;
	}
#endif
	pthread_rwlock_unlock(&thread->_lock_events);
	return ret;
}
