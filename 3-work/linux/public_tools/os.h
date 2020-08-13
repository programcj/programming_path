/*
 * os.h
 *
 *  Created on: 2020年7月31日
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

#ifndef SRC_SRC_OS_H_
#define SRC_SRC_OS_H_

#ifdef __cplusplus
extern "C"
{
#endif

int os_dir_stat(const char *path, unsigned long *total, unsigned long *use, unsigned long *free, int *used);

int os_net_ip(const char *eth_inf, char *ip, char *mask);

int os_net_mac(const char *eth_inf, char mac[18]);

int os_net_default_gw(char gw[20]);

struct eth_info
{
	char name[20];
	char mac[20]; //
	char ipaddress[16];
	char netmask[16]; //
};

int os_net_get_ifname(struct eth_info *eth, int len);

int os_cpu_count();

void os_name_get(char osname[50]);

//判断是否是虚拟网卡
int os_net_isvirtual(const char *name);

//netstat -antue4op
struct netstat_item
{
	char proto[10];
	unsigned long rxq; //Recv-Q
	unsigned long txq; //Send-Q
	char local_addr[30]; //Local Address
	char foreign_addr[30]; //Foreign Address
	char state[15]; //ESTABLISHED
	char timers[64];
	unsigned long inode; //readlink("/proc/8679/fd/6", buff, sizeof(buff));
};

int os_net_status(void (*bkfun)(struct netstat_item *item, void *usr), void *usr);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SRC_OS_H_ */
