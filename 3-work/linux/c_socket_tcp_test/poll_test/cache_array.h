/*
 * carray.h
 *
 *  Created on: 2019年6月16日
 *      Author: cc
 */

#ifndef _CACHE_ARRAY_H
#define _CACHE_ARRAY_H

#include <stdlib.h>
#include <string.h>

//缓存数组
#if 0
struct cache_array {
	unsigned char *data;
	unsigned char *use_sptr; //start
	unsigned char *use_eptr;//end
	int size;
	int point_end;
	int remain_len;
};
#else
struct cache_array {
	unsigned char *data;
	int size;

	unsigned char *use_sptr; //start
	unsigned char *use_eptr; //end
};
#endif

static inline int cache_array_init(struct cache_array *array, int size) {
	memset(array, 0, sizeof(typeof(*array)));
	array->size = size;
	array->data = (unsigned char*) malloc(size);
	array->use_sptr = array->use_eptr = array->data;
	return 0;
}

static inline void cache_array_destory(struct cache_array *array) {
	if (array->data)
		free(array->data);
	memset(array, 0, sizeof(typeof(*array)));
}

#if 0
//已经使用的长度
static inline int cache_array_uselen(struct cache_array *array) {
	return array->point_end;
}

//剩余长度
static inline int cache_array_remainlen(struct cache_array *array) {
	return array->size - array->point_end;
}

//剩余位置指针
static inline unsigned char *cache_array_remainptr(struct cache_array *array) {
	return array->data + array->point_end;
}

//增加数据
static inline int cache_array_remaindata_append(struct cache_array *array,
		void *data, int size) {
	if (size + array->point_end > array->size)
	size = array->size - array->point_end;

	memcpy(array->data + array->point_end, data, size);
	array->point_end += size;
	return size;
}

//增加使用计数
static inline int cache_array_uselen_add(struct cache_array *array, int len) {
	array->point_end += len;
	return array->point_end;
}

//减使用计数
static inline void cache_array_usedlen_sub(struct cache_array *array, int len) {
	if (len > array->point_end)
	len = array->point_end;
	memcpy(array->data, array->data + len, array->point_end - len);
	array->point_end = array->point_end - len;
}

//使用数据的开始指针
static inline void *cache_array_useptr(struct cache_array *array) {
	return (void*) array->use_sptr;
}

#else

//已经使用的长度
static inline int cache_array_uselen(struct cache_array *array) {
	return array->use_eptr - array->use_sptr;
}

//剩余位置指针
static inline unsigned char *cache_array_remainptr(struct cache_array *array) {
	return array->use_eptr;
}

//剩余长度, 可以直接复制到remptr的剩余长度
static inline int cache_array_remainlen(struct cache_array *array) {
	return array->size - (array->use_eptr - array->data);
}

static inline int cache_array_remainlen_tran(struct cache_array *array) {
	if (array->use_sptr != array->data) {
		memcpy(array->data, array->use_sptr, array->use_eptr - array->use_sptr); //转移
		array->use_eptr -= array->use_sptr - array->data;
		array->use_sptr = array->data;
	}
	return array->size - (array->use_eptr - array->data);
}

#include <stdio.h>
//增加数据
static inline int cache_array_remaindata_append(struct cache_array *array,
		void *data, int size) {
	int remlen = cache_array_remainlen(array);

	if (remlen < size && array->use_sptr != array->data) { //重新计算数据
		memcpy(array->data, array->use_sptr, array->use_eptr - array->use_sptr); //转移
		array->use_eptr -= array->use_sptr - array->data;
		array->use_sptr = array->data;
	}
	remlen = cache_array_remainlen(array);
	if (remlen < size)
		size = remlen;

	memcpy(array->use_eptr, data, size);
	array->use_eptr += size;

	if (array->use_eptr - array->data > array->size) {
		fprintf(stderr, "cache_array_remaindata_append err.....:\n");
		exit(-1);
	}
	return size;
}

//增加使用计数
static inline int cache_array_uselen_add(struct cache_array *array, int len) {
	array->use_eptr += len;

	if (array->use_eptr - array->data > array->size) {
		fprintf(stderr, "cache_array_uselen_add err.....:\n");
		exit(-1);
	}
	return array->use_eptr - array->use_sptr;
}

//减使用计数
static inline void cache_array_usedlen_sub(struct cache_array *array, int len) {
	array->use_sptr += len;
}

//使用数据的开始指针
static inline void *cache_array_useptr(struct cache_array *array) {
	return (void*) array->use_sptr;
}

#endif

#endif /* _cache_arrayH */
