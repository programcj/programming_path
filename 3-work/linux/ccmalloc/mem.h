/*
 * zw_mem.h
 *
 *  Created on: 2017年12月14日
 *      Author: cj
 *
 *		需要使用下面的宏定义制定函数
 *         #define		malloc(size)		mem_malloc(size)
 *		　#define		free(ptr)			mem_free(ptr)
 * 		#define		realloc				err
*			#define		calloc					err
 */

#ifndef HAL_MEM_H_
#define HAL_MEM_H_

#include <stddef.h>

//输出内存申请的列表数据
extern void mem_debug();

/*
 * 申请内存:可以制定一些调试信息
 */
extern void *_mem_malloc(size_t size, const char *file, const char *function, int line);

extern void *mem_realloc(void *__ptr, size_t __size, const char *file, const char *function,
		int line);

/**
 * 释放内存
 */
extern void mem_free(void *ptr);

//编译选项添加 -Wl,-wrap=malloc 可以替换系统stdlib库的malloc，但是这样不能知道调试信息，只能知道内存指针
extern void *__wrap_malloc(size_t c);

// -Wl,-wrap=free
extern void __wrap_free(void *ptr);

#define	mem_malloc(size)	  _mem_malloc(size, __FILE__, __FUNCTION__, __LINE__)

#endif /* HAL_MEM_H_ */
