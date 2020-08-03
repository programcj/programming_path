/*
 * os_proc.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include "os_proc.h"

static int read_to_buf(const char *filename, void *buf, int bufsize)
{
	int fd;
	/* open_read_close() would do two reads, checking for EOF.
	 * When you have 10000 /proc/$NUM/stat to read, it isn't desirable */
	ssize_t ret = -1;
	fd = open(filename, O_RDONLY);
	if (fd >= 0)
	{
		ret = read(fd, buf, bufsize - 1);
		close(fd);
	}
	((char *) buf)[ret > 0 ? ret : 0] = '\0';
	return ret;
}

static unsigned long os_proc_get_value_int(const char *path, const char *name)
{
	FILE *fp = fopen(path, "r");
	char buff[1024];
	char *p = NULL;
	unsigned long value = 0;
	if (fp)
	{
		while (fgets(buff, sizeof(buff) - 1, fp))
		{
			p = buff;
			if (strncmp(buff, name, strlen(name)) == 0)
			{
				p += strlen(name);

				if (*p != ':')
					continue;
				p++;
				while (!isdigit(*p) && *p)
					p++;
				value = atol(p);
				break;
			}
		}
		fclose(fp);
	}
	return value;
}

static void proc_info_format_stat(char *statbuf, struct proc_info *info)
{
	int n;
	char *cp = strrchr(statbuf, ')');
	char *comm = strchr(statbuf, '(');
	comm++;
	*cp = 0;

	if (strlen(info->cmdline) == 0)
		sprintf(info->comm, "[%s]", comm);
	else
		strcpy(info->comm, comm);

	if (cp)
		cp += 2;
	n = sscanf(cp,
				"%c %u " /* state, ppid */
							"%u %u %d %*s " /* pgid, sid, tty, tpgid */
							"%*s %*s %*s %*s %*s " /* flags, min_flt, cmin_flt, maj_flt, cmaj_flt */
							"%lu %lu " /* utime, stime */
							"%*s %*s %*s " /* cutime, cstime, priority */
							"%ld " /* nice */
							"%*s %*s " /* timeout, it_real_value */
							"%lu " /* start_time */
							"%lu " /* vsize */
							"%lu " /* rss */
				,
				info->state, &info->ppid,
				&info->pgid, &info->sid, &info->tty,
				&info->utime, &info->stime,
				&info->tasknice,
				&info->start_time,
				&info->vsz,
				&info->rss);
	info->vsz = info->vsz >> 10;
	info->ticks = info->utime + info->stime;
}

void proc_destroy(struct list_head *plist)
{
	struct list_head *lp, *ln;
	struct proc_info *pinfo = NULL;

	list_for_each_prev_safe(lp, ln, plist)
	{
		pinfo = list_entry(lp, struct proc_info, list);
//		if (strlen(pinfo->cmdline) > 0)
//			printf("%d CPU:%lu%% %lu%% %.20s\n", pinfo->pid, pinfo->use_cpu, pinfo->use_mem,
//						pinfo->cmdline);
		list_del(lp);
		free(pinfo);
	}
}

void proc_scan(struct list_head *plist)
{
	unsigned long MemTotal = os_proc_get_value_int("/proc/meminfo", "MemTotal");
	{
		//struct list_head list;
		struct list_head *lp, *ln;
		int n;
		//INIT_LIST_HEAD(&list);

		DIR * dir = opendir("/proc");
		int ret;
		struct dirent entry, *dp;
		char filename[sizeof("/proc/%u/task/%u/cmdline") + sizeof(int) * 3 * 2];
		char buff[1024];
		int _s;
		struct proc_info *pinfo = NULL;
		int procscount = 0;
		if (dir)
		{
			while (!readdir_r(dir, &entry, &dp) && dp)
			{
				//printf("%s\n", entry.d_name);
				unsigned pid = strtoul(entry.d_name, NULL, 10);
				if (pid == 0)
					continue;

				sprintf(filename, "/proc/%u/cmdline", pid);
				memset(buff, 0, sizeof(buff));
				n = read_to_buf(filename, buff, sizeof(buff));
				if(n<0)
					continue;

				pinfo = (struct proc_info*) calloc(sizeof(struct proc_info), 1);
				if(!pinfo)
					continue;

				pinfo->pid = pid;
				if (n > 0)
				{
					_s = 0;
					do
					{
						n--;
						if (buff[n] == 0 && _s == 0)
							continue;
						_s = 1;
						if ((unsigned char) (buff[n]) < ' ')
							buff[n] = ' ';
					} while (n);
					strcpy(pinfo->cmdline, buff);
				}

				sprintf(filename, "/proc/%u/exe", pid);
				memset(buff, 0, sizeof(buff));
				ret = readlink(filename, buff, sizeof(buff));
				if (ret != 0)
					if (EACCES == errno)
						sprintf(buff, "Permission denied");
				strcpy(pinfo->exepath, buff);

				sprintf(filename, "/proc/%u/stat", pid);
				n = read_to_buf(filename, buff, sizeof(buff));
				if (n <= 0) {
					printf("err....pid:%d,name:%s\n", pid, pinfo->cmdline);
					free(pinfo);
					continue;
				}
				proc_info_format_stat(buff, pinfo);

				sprintf(filename, "/proc/%u/status", pid);
				pinfo->VmRSS = os_proc_get_value_int(filename, "VmRSS");

				list_add_tail(&pinfo->list, plist);
				procscount++;
			}
			closedir(dir);

			//sleep(1);
			{
				int seconds_since_boot = 0;
				double up = 0;
				{
					//第一个参数是代表从系统启动到现在的时间(以秒为单位)：
					//第二个参数是代表系统空闲的时间(以秒为单位)：
					FILE* fp = fopen("/proc/uptime", "r");
					char buff[1024];

					if (fp)
					{
						fgets(buff, sizeof(buff), fp);
						fclose(fp);
						sscanf(buff, "%lf ", &up);
					}
				}
				//unsigned long long total_time; /* jiffies used by this process */

				list_for_each_prev_safe(lp, ln, plist)
				{
					pinfo = list_entry(lp, struct proc_info, list);
					seconds_since_boot = up;
					unsigned long pcpu = 0; /* scaled %cpu, 999 means 99.9% */
					unsigned long long seconds = 0; /* seconds of process life */
					int Hertz = 100;

					if (((unsigned long long) seconds_since_boot >= (pinfo->start_time / Hertz)))
					{
						seconds = ((unsigned long long) seconds_since_boot - (pinfo->start_time / Hertz));
					}

					if (seconds)
						pcpu = (pinfo->ticks * 1000ULL / Hertz) / seconds;
					pinfo->use_cpu = pcpu / 10U;
					pinfo->use_mem = 100 * (pinfo->VmRSS * 1.0 / MemTotal);
				}
			}

			{
//				list_for_each_prev_safe(lp, ln, plist)
//				{
//					pinfo = list_entry(lp, struct proc_info, list);
//
//					if (strlen(pinfo->cmdline) > 0)
//						printf("%d CPU:%lu%% %lu%% %.20s\n", pinfo->pid, pinfo->use_cpu, pinfo->use_mem,
//									pinfo->cmdline);
//					list_del(lp);
//					free(pinfo);
//				}
			}
		}
	}
}

