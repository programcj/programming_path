/*
 * zw_mem.c
 *
 *  Created on: 2017年12月14日
 *      Author: cj
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define THREAD_LOCK

#ifdef THREAD_LOCK
#include <pthread.h>
#endif

#include "mem.h"
///////////////////////////////////////////////////////////////////////////////
struct list_head {
	struct list_head *next, *prev;
};

static inline void __list_add(struct list_head *new, struct list_head *prev,
		struct list_head *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void __list_del(struct list_head *prev, struct list_head *next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry) {
	__list_del(entry->prev, entry->next);
	entry->next = (void *)0xFFFFFF1;
	entry->prev = (void *)0xFFFFFF2;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head) {
	__list_add(new, head->prev, head);
}

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
///////////////////////////////////////////////////////////////////////////////

static struct list_head _list_mem = { &_list_mem, &_list_mem };
static pthread_rwlock_t _lock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct mem {
	struct list_head list;
	void *ptr;
	const char *file;
	const char *fun;
	int line;
	int size;
} mem_t;

static void __mem_append(void *ptr, const char *file, const char *function, int line, int size);
static void _mem_remove(void *ptr, void (*_free)(void *));

static void _mem_append(mem_t *mem, void *ptr, const char *file, const char *function, int line,
		int size);

extern void *__real_malloc(size_t);
extern void __real_free(void *);

void *__wrap_malloc(size_t c) {
	void *ptr = (void*) __real_malloc(c);
	if (ptr) {
		mem_t *mem = NULL;
		mem = (mem_t*) __real_malloc(sizeof(mem_t));
		if (mem) {
			memset(mem, 0, sizeof(mem_t));
			_mem_append(mem, ptr, "mem.c", "malloc", 0, c);
		}
	}
	return ptr;
}

static void _mem_free(void *ptr) {
	__real_free(ptr);
}

void __wrap_free(void *ptr) {
	__real_free(ptr);
	_mem_remove(ptr, _mem_free);
}

void mem_debug() {
	struct list_head *p, *n;
	mem_t *mem;

#ifdef THREAD_LOCK
	pthread_rwlock_rdlock(&_lock);
#endif

	list_for_each_safe(p, n, &_list_mem)
	{
		mem = list_entry(p, mem_t, list);
		printf("%04X(%s, %s,%d):%d \n", (int) mem->ptr, mem->file, mem->fun, mem->line, mem->size);
		fflush(stdout);
	}

#ifdef THREAD_LOCK
	pthread_rwlock_unlock(&_lock);
#endif
}

static void _mem_append(mem_t *mem, void *ptr, const char *file, const char *function, int line,
		int size) {
#ifdef THREAD_LOCK
	pthread_rwlock_wrlock(&_lock);
#endif
	mem->ptr = ptr;
	mem->line = line;
	mem->fun = function;
	mem->file = file;
	mem->size = size;

	list_add_tail(&mem->list, &_list_mem);

#ifdef THREAD_LOCK
	pthread_rwlock_unlock(&_lock);
#endif
}

static void __mem_append(void *ptr, const char *file, const char *function, int line, int size) {
	mem_t *mem = NULL;
	mem = (mem_t*) malloc(sizeof(mem_t));
	if (mem) {
		memset(mem, 0, sizeof(mem_t));
		_mem_append(mem, ptr, file, function, line, size);
	}
}

static void _mem_remove(void *ptr, void (*_free)(void *)) {
	struct list_head *p, *n;
	mem_t *mem;

#ifdef THREAD_LOCK
	pthread_rwlock_wrlock(&_lock);
#endif

	list_for_each_safe(p, n, &_list_mem)
	{
		mem = list_entry(p, mem_t, list);
		if (mem->ptr == ptr) {
			list_del(&mem->list);
			_free(mem);
		}
	}

#ifdef THREAD_LOCK
	pthread_rwlock_unlock(&_lock);
#endif
}

void *_mem_malloc(size_t size, const char *file, const char *function, int line) {
	void *ptr = malloc(size);
	if (ptr) {
		__mem_append(ptr, file, function, line, size);
	}
	return ptr;
}

void *mem_realloc(void *__ptr, size_t __size, const char *file, const char *function, int line) {
	void *ptr = realloc(__ptr, __size);
	if (__ptr) {
		__mem_append(ptr, file, function, line, __size);
	}
	return ptr;
}

void mem_free(void *ptr) {
	if (ptr) {
		free(ptr);
		_mem_remove(ptr, free);
	}
}
