/*
 * mem_ex.h
 *
 *  Created on: 2018年4月16日
 *      Author: cj
 */

#ifndef MEM_EX_H_
#define MEM_EX_H_

#include "mem.h"

#define		malloc(size)		mem_malloc(size)
#define		free(ptr)			mem_free(ptr)
#define		realloc				err
#define		calloc					err

#endif /* MEM_EX_H_ */
