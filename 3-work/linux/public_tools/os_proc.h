/*
 * os_proc.h
 *
 *  Created on: 2020年7月30日
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

#ifndef SRC_SRC_OS_PROC_H_
#define SRC_SRC_OS_PROC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "list.h"

struct proc_info
{
	struct list_head list;
	unsigned pid;
	char cmdline[1024];
	char exepath[1024];
	char comm[20];

	char state[4]; // /proc/<pid>/stat
	unsigned ppid;
	unsigned pgid;
	unsigned sid;
	int tty;
	unsigned long utime;
	unsigned long stime;
	long tasknice; //任务的静态优先级。
	unsigned long start_time; //该任务启动的时间，单位为 jiffies。
	unsigned long vsz; //该任务的虚拟地址空间大小
	unsigned long rss; //该任务当前驻留物理地址空间的大小
	long VmRSS;
	unsigned long ticks; //utime+stime

	unsigned long use_mem;
	unsigned long use_cpu;
};

void proc_scan(struct list_head *plist);
void proc_destroy(struct list_head *plist);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SRC_OS_PROC_H_ */
