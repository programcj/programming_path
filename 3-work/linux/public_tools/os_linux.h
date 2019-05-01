/*
 * os_linux.h
 *
 *  Created on: 2017年9月15日
 *      Author: cj
 *      Email: 593184917@qq.com
 */

#ifndef SRC_PUBLIC_OS_LINUX_H_
#define SRC_PUBLIC_OS_LINUX_H_

/**
 * //tools_write_file("/proc/sys/vm/drop_caches", "1"); //清理Linux缓存
 */
void os_linux_drop_caches(int value);

/****************
 * busybox -> # include <sys/sysinfo.h> struct sysinfo info; sysinfo(&info);
void _meminfo(){
	unsigned long kb_main_cached=0; //Cached
	unsigned long kb_main_free=0;  //MemFree
	unsigned long kb_main_total=0; //MemTotal
	unsigned long kb_main_buffers=0;//Buffers

	os_proc_meminfo_read("Cached", &kb_main_cached);
	os_proc_meminfo_read("MemFree", &kb_main_free);
	os_proc_meminfo_read("MemTotal", &kb_main_total);
	os_proc_meminfo_read("Buffers", &kb_main_buffers);

	log_d("(kB)Total:%d, Use:%d, Free:%d, Buff:%d, Cache:%d",kb_main_total,kb_main_total-kb_main_free,
			kb_main_free,kb_main_buffers, kb_main_cached);
}
***/
int os_proc_meminfo_read(const char *name, unsigned long *value);

int os_linux_callcmd(int *rfd, int *wfd, char *cmdargs[]);

int io_nonblock(int fd, int flag);


#endif /* SRC_PUBLIC_OS_LINUX_H_ */
