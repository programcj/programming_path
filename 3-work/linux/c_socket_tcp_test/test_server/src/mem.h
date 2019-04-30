/*
 * zw_mem.h
 *
 *  Created on: 2017年12月14日
 *      Author: cj
 */

#ifndef HAL_MEM_H_
#define HAL_MEM_H_

#include <stddef.h>
//memory
void mem_init();

void mem_debug();

extern void *_mem_malloc(size_t size, const char *file, const char *function,
		int line);

extern void *mem_realloc(void *__ptr, size_t __size, const char *file,
		const char *function, int line);

extern void mem_free(void *ptr);

#define	mem_malloc(size)	  _mem_malloc(size, __FILE__, __FUNCTION__, __LINE__)

#endif /* HAL_MEM_H_ */
