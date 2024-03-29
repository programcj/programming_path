/*
 * os.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <ctype.h>

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include <sys/prctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/dir.h>
#include <dirent.h>
#include <errno.h>

#include "os.h"

static unsigned long kscale(unsigned long b, unsigned long bs)
{
	return (b * (unsigned long long) bs + 1024 / 2) / 1024;
}

int os_dir_stat(const char *path, unsigned long *total, unsigned long *use, unsigned long *free, int *used)
{
	struct statfs s;
	unsigned long blocks_used;
	unsigned blocks_percent_used;

	if (0 != statfs(path, &s))
		return -1;

	if (s.f_blocks > 0)
	{
		blocks_used = s.f_blocks - s.f_bfree;
		blocks_percent_used = 0;
		if (blocks_used + s.f_bavail)
		{
			blocks_percent_used = (blocks_used * 100ULL
						+ (blocks_used + s.f_bavail) / 2
						) / (blocks_used + s.f_bavail);
		}
		*total = kscale(s.f_blocks, s.f_bsize);
		*use = kscale(s.f_blocks - s.f_bfree, s.f_bsize);
		*free = kscale(s.f_bavail, s.f_bsize);
		*used = blocks_percent_used;
		return 0;
	}
	return -1;
}

// 获取本机ip
int os_net_ip(const char *eth_inf, char *ip, char *mask)
{
	int sd;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sd)
	{
		printf("socket error: %s\n", strerror(errno));
		return -1;
	}

	strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	// if error: No such device
	if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)
	{
		printf("ioctl error SIOCGIFADDR: %s\n", strerror(errno));
		close(sd);
		return -1;
	}
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	if (ip)
		inet_ntop(AF_INET, &sin.sin_addr, ip, 17);

	if (ioctl(sd, SIOCGIFNETMASK, &ifr) < 0)
	{
		printf("ioctl error SIOCGIFNETMASK: %s\n", strerror(errno));
		close(sd);
		return -1;
	}
	if (mask)
		inet_ntop(AF_INET, &((struct sockaddr_in *) &ifr.ifr_netmask)->sin_addr, mask, 17);
	close(sd);
	return 0;
}

// 获取本机mac
int os_net_mac(const char *eth_inf, char mac[18])
{
	struct ifreq ifr;
	int sd;

	bzero(&ifr, sizeof(struct ifreq));
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("get %s mac address socket creat error\n", eth_inf);
		return -1;
	}

	strncpy(ifr.ifr_name, eth_inf, sizeof(ifr.ifr_name) - 1);

	if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
	{
		printf("ioctl SIOCGIFHWADDR err:%s mac address error\n", eth_inf);
		close(sd);
		return -1;
	}

	snprintf(mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned char) ifr.ifr_hwaddr.sa_data[0],
				(unsigned char) ifr.ifr_hwaddr.sa_data[1],
				(unsigned char) ifr.ifr_hwaddr.sa_data[2],
				(unsigned char) ifr.ifr_hwaddr.sa_data[3],
				(unsigned char) ifr.ifr_hwaddr.sa_data[4],
				(unsigned char) ifr.ifr_hwaddr.sa_data[5]);
	close(sd);
	return 0;
}

int os_net_default_gw(char gw[20])
{
#define RTF_UP 0x0001		 /* route usable                 */
#define RTF_GATEWAY 0x0002   /* destination is a gateway     */
#define RTF_HOST 0x0004		 /* host entry (net otherwise)   */
#define RTF_REINSTATE 0x0008 /* reinstate route after tmout  */
#define RTF_DYNAMIC 0x0010   /* created dyn. (by redirect)   */
#define RTF_MODIFIED 0x0020  /* modified dyn. (by redirect)  */
#define RTF_MTU 0x0040		 /* specific MTU for this route  */

	char devname[64];
	unsigned long d, g, m;
	int flgs, ref, use, metric, mtu, win, ir;
	//struct sockaddr_in s_addr;
	struct in_addr dscaddr;
	struct in_addr gwaddr;
	struct in_addr mask;

	FILE *fp = fopen("/proc/net/route", "r");
	memset(gw, 0, 20);
	if (!fp)
		return -1;

	if (fscanf(fp, "%*[^\n]\n") < 0)
	{ /* Skip the first line. */
		goto _ERROR;
	}
	while (1)
	{
		int r;
		r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n", devname, &d, &g, &flgs,
					&ref, &use, &metric, &m, &mtu, &win, &ir);
		//printf("r=%d\n", r);

		if (r != 11)
		{
			if ((r < 0) && feof(fp))
			{ /* EOF with no (nonspace) chars read. */
				break;
			}
			continue;
		}
		if (!(flgs & RTF_UP))
		{ /* Skip interfaces that are down. */
			continue;
		}
		dscaddr.s_addr = d;
		gwaddr.s_addr = g;
		mask.s_addr = m;
		if (flgs & RTF_GATEWAY)
		{
			//printf("UG:");
			inet_ntop(AF_INET, &gwaddr, gw, 20);
			break;
		}
//		printf("%s ", devname);
//		printf("%s->", inet_ntoa(dscaddr));
//		printf("%s, ", inet_ntoa(gwaddr));
//		printf("%s ", inet_ntoa(mask));
	}

	_ERROR:
	fclose(fp);
	return 0;
}

int os_net_get_ifname(struct eth_info *eth, int len)
{
	int num = 0;
	char buf[1024];
	char name[10];
	FILE *fp = fopen("/proc/net/dev", "r");
	if (fp)
	{
		/* skip headers */
		fgets(buf, sizeof(buf), fp);
		fgets(buf, sizeof(buf), fp);

		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			char *ptr = strchr(buf, ':');
			if (ptr == NULL ||
						(*ptr++ = 0, sscanf(buf, "%s", name) != 1)
						)
			{
				break;
			}

			if (strcmp(name, "lo") == 0)
				continue;

			//判断是否是虚拟网卡
			if (os_net_isvirtual(name))
				continue;

			strcpy(eth[num].name, name);
			os_net_mac(name, eth[num].mac);
			os_net_ip(name, eth[num].ipaddress, eth[num].netmask);
			num++;
		}
		fclose(fp);
	}
#if 0
	int fd;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	return -1;

	struct ifreq buf[20];
	struct ifconf ifc;

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_req = buf;
	if (ioctl(fd, SIOCGIFCONF, (char *) &ifc) < 0)
	{
		close(fd);
		return -1;
	}
	int num = ifc.ifc_len / sizeof(struct ifreq);
	int i;
	int ret;
	char name[20];

	for (i = 0; i < num && i < len; i++)
	{
		strncpy(name, buf[i].ifr_name, sizeof(name) - 1);
		strcpy(eth[i].name, name);

		ret = ioctl(fd, SIOCGIFFLAGS, &buf[i]);
		if (ret < 0)
		continue;

		ret = ioctl(fd, SIOCGIFHWADDR, &buf[i]);
		if (ret < 0)
		continue;
		sprintf(eth[i].mac, "%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned char) buf[i].ifr_hwaddr.sa_data[0],
					(unsigned char) buf[i].ifr_hwaddr.sa_data[1],
					(unsigned char) buf[i].ifr_hwaddr.sa_data[2],
					(unsigned char) buf[i].ifr_hwaddr.sa_data[3],
					(unsigned char) buf[i].ifr_hwaddr.sa_data[4],
					(unsigned char) buf[i].ifr_hwaddr.sa_data[5]);

		ret = ioctl(fd, SIOCGIFADDR, &buf[i]);
		if (ret < 0)
		continue;
		inet_ntop(AF_INET, &((struct sockaddr_in*) (&buf[i].ifr_addr))->sin_addr, eth[i].ipaddress, 17);

		ret = ioctl(fd, SIOCGIFNETMASK, &buf[i]);
		if (ret < 0)
		continue;

		inet_ntop(AF_INET, &((struct sockaddr_in*) (&buf[i].ifr_netmask))->sin_addr, eth[i].netmask, 17);
	}
	close(fd);
#endif
	return num;
}

int os_net_isvirtual(const char *name)
{
	DIR *dir = opendir("/sys/devices/virtual/net/");
	int ret = 0;
	if (dir)
	{
		struct dirent *ditem = NULL;
		while (NULL != (ditem = readdir(dir)))
		{
			if (strcmp(ditem->d_name, name) == 0)
			{
				ret = 1;
				break;
			}
		}
		closedir(dir);
	}
	return ret;
}

int os_cpu_count()
{
	return get_nprocs();
}

void os_name_get(char osname[50])
{
	if (access("/etc/openwrt_release", F_OK) == 0)
	{
		strcpy(osname, "OpenWrt");
	}

	if (access("/etc/os-release", F_OK) == 0)
	{
		FILE *fp = fopen("/etc/os-release", "r");
		char PRETTY_NAME[50];
		char *p = NULL;

		memset(PRETTY_NAME, 0, sizeof(PRETTY_NAME));
		if (fp)
		{
			char buff[200];
			while (fgets(buff, sizeof(buff), fp))
			{
				if (strncmp(buff, "PRETTY_NAME=", strlen("PRETTY_NAME=")) == 0)
				{
					p = buff;
					p += strlen("PRETTY_NAME=") + 1;
					if (*p == '\"')
						p++;
					strncpy(PRETTY_NAME, p, 50 - 1);
					p = strchr(PRETTY_NAME, '\"');
					if (!p)
						p = strchr(PRETTY_NAME, '\r');
					if (!p)
						p = strchr(PRETTY_NAME, '\n');
					if (p)
						*p = 0;
					break;
				}
			}
			fclose(fp);
		}
		strcpy(osname, PRETTY_NAME);
	}
}

enum
{
	TCP_ESTABLISHED = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECV,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_TIME_WAIT,
	TCP_CLOSE,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_LISTEN,
	TCP_CLOSING /* now a valid state */
};

static const char * const tcp_state[] =
			{
						"",
						"ESTABLISHED",
						"SYN_SENT",
						"SYN_RECV",
						"FIN_WAIT1",
						"FIN_WAIT2",
						"TIME_WAIT",
						"CLOSE",
						"CLOSE_WAIT",
						"LAST_ACK",
						"LISTEN",
						"CLOSING"
			};

static void _tcp_do_one(int lnr, const char *line, struct netstat_item *pitem)
{
	if (lnr == 0)
		return;
	unsigned long rxq, txq, time_len, retr, inode;
	int num, local_port, rem_port, d, state, uid, timer_run, timeout;
	char rem_addr[128], local_addr[128], timers[64], buffer[1024], more[512];
//	struct aftype *ap;
#if HAVE_AFINET6
	struct sockaddr_in6 localaddr, remaddr;
	char addr6[INET6_ADDRSTRLEN];
	struct in6_addr in6;
	extern struct aftype inet6_aftype;
#else
	struct sockaddr_in localaddr, remaddr;
#endif

	if (lnr == 0)
		return;

	num = sscanf(line,
				"%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
				&d, local_addr, &local_port, rem_addr, &rem_port, &state,
				&txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);

	if (strlen(local_addr) > 8)
	{
#if HAVE_AFINET6
		/* Demangle what the kernel gives us */
		sscanf(local_addr, "%08X%08X%08X%08X",
					&in6.s6_addr32[0], &in6.s6_addr32[1],
					&in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet6_aftype.input(1, addr6, (struct sockaddr *) &localaddr);
		sscanf(rem_addr, "%08X%08X%08X%08X",
					&in6.s6_addr32[0], &in6.s6_addr32[1],
					&in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet6_aftype.input(1, addr6, (struct sockaddr *) &remaddr);
		localaddr.sin6_family = AF_INET6;
		remaddr.sin6_family = AF_INET6;
#endif
	}
	else
	{
		sscanf(local_addr, "%X",
					&((struct sockaddr_in *) &localaddr)->sin_addr.s_addr);
		sscanf(rem_addr, "%X",
					&((struct sockaddr_in *) &remaddr)->sin_addr.s_addr);
		((struct sockaddr *) &localaddr)->sa_family = AF_INET;
		((struct sockaddr *) &remaddr)->sa_family = AF_INET;
	}

	if (num < 11)
	{
		fprintf(stderr, "warning, got bogus tcp line.\n");
		return;
	}
//	if ((ap = get_afntype(((struct sockaddr *) &localaddr)->sa_family)) == NULL)
//	{
//		fprintf(stderr, _("netstat: unsupported address family %d !\n"),
//					((struct sockaddr *) &localaddr)->sa_family);
//		return;
//	}
	if (state == TCP_LISTEN)
	{
		time_len = 0;
		retr = 0L;
		rxq = 0L;
		txq = 0L;
	}

//	safe_strncpy(local_addr, ap->sprint((struct sockaddr *) &localaddr,
//				flag_not), sizeof(local_addr));
//	safe_strncpy(rem_addr, ap->sprint((struct sockaddr *) &remaddr, flag_not),
//				sizeof(rem_addr));

	inet_ntop(AF_INET, &localaddr.sin_addr.s_addr, local_addr, sizeof(local_addr));
	inet_ntop(AF_INET, &remaddr.sin_addr.s_addr, rem_addr, sizeof(rem_addr));

//	if (flag_all || (flag_lst && !rem_port) || (!flag_lst && rem_port))
	{
		//snprintf(buffer, sizeof(buffer), "%d", htons(local_port));
//		if ((strlen(local_addr) + strlen(buffer)) > 22)
//			local_addr[22 - strlen(buffer)] = '\0';
//		strcat(local_addr, ":");
//		strcat(local_addr, buffer);
//		snprintf(buffer, sizeof(buffer), "%d", htons(rem_port));
//		if ((strlen(rem_addr) + strlen(buffer)) > 22)
//			rem_addr[22 - strlen(buffer)] = '\0';

//		strcat(rem_addr, ":");
//		strcat(rem_addr, buffer);
		timers[0] = '\0';
//
//		if (flag_opt)
		switch (timer_run)
		{
			case 0:
				snprintf(timers, sizeof(timers), "off (0.00/%ld/%d)", retr, timeout);
			break;

			case 1:
				snprintf(timers, sizeof(timers), "on (%2.2f/%ld/%d)",
							(double) time_len / HZ, retr, timeout);
			break;

			case 2:
				snprintf(timers, sizeof(timers), "keepalive (%2.2f/%ld/%d)",
							(double) time_len / HZ, retr, timeout);
			break;

			case 3:
				snprintf(timers, sizeof(timers), "timewait (%2.2f/%ld/%d)",
							(double) time_len / HZ, retr, timeout);
			break;

			default:
				snprintf(timers, sizeof(timers), "unkn-%d (%2.2f/%ld/%d)",
							timer_run, (double) time_len / HZ, retr, timeout);
			break;
		}
//		printf("tcp   %6ld %6ld %-23s %-23s %-12s [%lu]\n",
//					rxq, txq, local_addr, rem_addr, tcp_state[state], inode);
		strcpy(pitem->proto, "tcp");
		pitem->rxq = rxq;
		pitem->txq = txq;
		strncpy(pitem->local_addr, local_addr, 30);
		pitem->local_port = htons(local_port);
		strncpy(pitem->foreign_addr, rem_addr, 30);
		pitem->foreign_port = htons(rem_port);
		strncpy(pitem->state, tcp_state[state], 15);
		pitem->inode = inode;
		strncpy(pitem->timers, timers, 64);
//		finish_this_one(uid, inode, timers);
	}
}

static void _udp_do_one(int lnr, const char *line, struct netstat_item *pitem)
{
	char buffer[8192], local_addr[64], rem_addr[64];
	char *udp_state, timers[64], more[512];
	int num, local_port, rem_port, d, state, timer_run, uid, timeout;
#if HAVE_AFINET6
	struct sockaddr_in6 localaddr, remaddr;
	char addr6[INET6_ADDRSTRLEN];
	struct in6_addr in6;
	extern struct aftype inet6_aftype;
#else
	struct sockaddr_in localaddr, remaddr;
#endif
//	struct aftype *ap;
	unsigned long rxq, txq, time_len, retr, inode;

	if (lnr == 0)
		return;

	more[0] = '\0';
	num = sscanf(line,
				"%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
				&d, local_addr, &local_port,
				rem_addr, &rem_port, &state,
				&txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);

	if (strlen(local_addr) > 8)
	{
#if HAVE_AFINET6
		sscanf(local_addr, "%08X%08X%08X%08X",
					&in6.s6_addr32[0], &in6.s6_addr32[1],
					&in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet6_aftype.input(1, addr6, (struct sockaddr *) &localaddr);
		sscanf(rem_addr, "%08X%08X%08X%08X",
					&in6.s6_addr32[0], &in6.s6_addr32[1],
					&in6.s6_addr32[2], &in6.s6_addr32[3]);
		inet_ntop(AF_INET6, &in6, addr6, sizeof(addr6));
		inet6_aftype.input(1, addr6, (struct sockaddr *) &remaddr);
		localaddr.sin6_family = AF_INET6;
		remaddr.sin6_family = AF_INET6;
#endif
	}
	else
	{
		sscanf(local_addr, "%X",
					&((struct sockaddr_in *) &localaddr)->sin_addr.s_addr);
		sscanf(rem_addr, "%X",
					&((struct sockaddr_in *) &remaddr)->sin_addr.s_addr);
		((struct sockaddr *) &localaddr)->sa_family = AF_INET;
		((struct sockaddr *) &remaddr)->sa_family = AF_INET;
	}

	retr = 0L;
	//if (!flag_opt)
	//	more[0] = '\0';

	if (num < 10)
	{
		fprintf(stderr, "warning, got bogus udp line.\n");
		return;
	}
//	if ((ap = get_afntype(((struct sockaddr *) &localaddr)->sa_family)) == NULL)
//	{
//		fprintf(stderr, _("netstat: unsupported address family %d !\n"),
//					((struct sockaddr *) &localaddr)->sa_family);
//		return;
//	}
	switch (state)
	{
		case TCP_ESTABLISHED:
			udp_state = "ESTABLISHED";
		break;

		case TCP_CLOSE:
			udp_state = "";
		break;

		default:
			udp_state = "UNKNOWN";
		break;
	}

#if HAVE_AFINET6
#define notnull(A) (((A.sin6_family == AF_INET6) && \
	 ((A.sin6_addr.s6_addr32[0]) ||            \
	  (A.sin6_addr.s6_addr32[1]) ||            \
	  (A.sin6_addr.s6_addr32[2]) ||            \
	  (A.sin6_addr.s6_addr32[3]))) ||          \
	((A.sin6_family == AF_INET) &&             \
	 ((struct sockaddr_in *) &A)->sin_addr.s_addr))
#else
#define notnull(A) (A.sin_addr.s_addr)
#endif

//	if (flag_all || (notnull(remaddr) && !flag_lst) || (!notnull(remaddr) && flag_lst))
	{
		inet_ntop(AF_INET, &localaddr.sin_addr.s_addr, local_addr, sizeof(local_addr));
		//safe_strncpy(local_addr, ap->sprint((struct sockaddr *) &localaddr, flag_not), sizeof(local_addr));
//		snprintf(buffer, sizeof(buffer), "%d", htons(local_port));
//		if ((strlen(local_addr) + strlen(buffer)) > 22)
//			local_addr[22 - strlen(buffer)] = '\0';
//		strcat(local_addr, ":");
//		strcat(local_addr, buffer);

//		snprintf(buffer, sizeof(buffer), "%d", htons(rem_port));
		inet_ntop(AF_INET, &remaddr.sin_addr.s_addr, rem_addr, sizeof(rem_addr));
		//safe_strncpy(rem_addr, ap->sprint((struct sockaddr *) &remaddr,	flag_not), sizeof(rem_addr));
//		if ((strlen(rem_addr) + strlen(buffer)) > 22)
//			rem_addr[22 - strlen(buffer)] = '\0';
//		strcat(rem_addr, ":");
//		strcat(rem_addr, buffer);

		timers[0] = '\0';
//		if (flag_opt)
		switch (timer_run)
		{
			case 0:
				snprintf(timers, sizeof(timers), "off (0.00/%ld/%d)", retr, timeout);
			break;

			case 1:
				case 2:
				snprintf(timers, sizeof(timers), "on%d (%2.2f/%ld/%d)", timer_run, (double) time_len / 100, retr, timeout);
			break;

			default:
				snprintf(timers, sizeof(timers), "unkn-%d (%2.2f/%ld/%d)", timer_run, (double) time_len / 100,
							retr, timeout);
			break;
		}
		//printf("udp   %6ld %6ld %-23s %-23s %-12s %lu\n", rxq, txq, local_addr, rem_addr, udp_state, inode);
		//finish_this_one(uid, inode, timers);

		strcpy(pitem->proto, "udp");
		pitem->rxq = rxq;
		pitem->txq = txq;
		strncpy(pitem->local_addr, local_addr, 30);
		pitem->local_port = htons(local_port);
		strncpy(pitem->foreign_addr, rem_addr, 30);
		pitem->foreign_port = htons(rem_port);
		strncpy(pitem->state, tcp_state[state], 15);
		pitem->inode = inode;
		strncpy(pitem->timers, timers, 64);
	}
}

void _show_netstat(struct netstat_item *pitem, void *usr)
{
	printf("%s %6lu %6lu %-23s %-23s %-12s [%lu] %s\n", pitem->proto, pitem->rxq, pitem->txq, pitem->local_addr, pitem->foreign_addr, pitem->state,
				pitem->inode, pitem->timers);
}

int os_net_status(void (*bkfun)(struct netstat_item *pitem, void *usr), void *usr)
{
	char buffer[8192];
	int lnr;
	struct netstat_item item;
	{
		FILE *procinfo = fopen("/proc/net/tcp", "r");
		lnr = 0;
		do
		{
			if (fgets(buffer, sizeof(buffer), procinfo))
			{
				memset(&item, 0, sizeof(item));
				_tcp_do_one(lnr++, buffer, &item);
				if (strlen(item.proto) > 0 && bkfun)
					bkfun(&item, usr);
			}
		} while (!feof(procinfo));
		fclose(procinfo);
	}
	{
		FILE *procinfo = fopen("/proc/net/udp", "r");
		lnr = 0;
		do
		{
			if (fgets(buffer, sizeof(buffer), procinfo))
			{
				memset(&item, 0, sizeof(item));
				_udp_do_one(lnr++, buffer, &item);
				if (strlen(item.proto) > 0 && bkfun)
					bkfun(&item, usr);
			}
		} while (!feof(procinfo));
		fclose(procinfo);
	}
	return 0;
}

int os_net_rx_tx(struct net_flow *flow, int count)
{
	FILE *fp = fopen("/proc/net/dev", "r");
	char buf[1024];
	char name[50];
	unsigned long rx_bytes, rx_packets, rx_errs, rx_drops,
				rx_fifo, rx_frame,
				tx_bytes, tx_packets, tx_errs, tx_drops,
				tx_fifo, tx_colls, tx_carrier, rx_multi;
	int type;
	int i = 0;

	if (fp)
	{
		/* skip headers */
		fgets(buf, sizeof(buf), fp);
		fgets(buf, sizeof(buf), fp);

		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			char *ptr;

			/*buf[sizeof(buf) - 1] = 0; - fgets is safe anyway */
			ptr = strchr(buf, ':');
			if (ptr == NULL ||
						(*ptr++ = 0, sscanf(buf, "%s", name) != 1)
						)
			{
				break;
			}
			if (sscanf(ptr, "%lu%lu%lu%lu%lu%lu%lu%*d%lu%lu%lu%lu%lu%lu%lu",
						&rx_bytes, &rx_packets, &rx_errs, &rx_drops,
						&rx_fifo, &rx_frame, &rx_multi,
						&tx_bytes, &tx_packets, &tx_errs, &tx_drops,
						&tx_fifo, &tx_colls, &tx_carrier) != 14)
				continue;
			if (strcmp(name, "lo") == 0)
				continue;
			//判断是否是虚拟网卡
			if (os_net_isvirtual(name))
				continue;
			if (i < count)
			{
				strncpy(flow[i].name, name, 10);
				flow[i].rx_bytes = rx_bytes;
				flow[i].tx_bytes = tx_bytes;
				i++;
			}
			else
			{
				break;
			}
		}
		fclose(fp);
	}
	return i;
}

void os_mem_info(struct mem_info *minfo)
{
	FILE *file = fopen("/proc/meminfo", "r");
	char buffer[128];
	unsigned long long int totalMem = 0;
	unsigned long long int freeMem = 0;
	unsigned long long int sharedMem = 0;
	unsigned long long int buffersMem = 0;
	unsigned long long int cachedMem = 0;
	unsigned long long int totalSwap = 0;
	unsigned long long int swapFree = 0;
	unsigned long long int usedMem = 0;
	unsigned long long int usedSwap = 0;
	unsigned long long int shmem = 0;
	unsigned long long int sreclaimable = 0;
	unsigned long kb_slab = 0;
	if (file)
	{
		while (fgets(buffer, 128, file))
		{
			if (strncmp(buffer, "MemTotal:", strlen("MemTotal:")) == 0)
				sscanf(buffer + strlen("MemTotal:"), " %32llu kB", &totalMem);
			if (strncmp(buffer, "MemFree:", strlen("MemFree:")) == 0)
				sscanf(buffer + strlen("MemFree:"), " %32llu kB", &freeMem);
			if (strncmp(buffer, "MemShared:", strlen("MemShared:")) == 0)
				sscanf(buffer + strlen("MemShared:"), " %32llu kB", &sharedMem);
			if (strncmp(buffer, "Buffers:", strlen("Buffers:")) == 0)
				sscanf(buffer + strlen("Buffers:"), " %32llu kB", &buffersMem);
			if (strncmp(buffer, "Cached:", strlen("Cached:")) == 0)
				sscanf(buffer + strlen("Cached:"), " %32llu kB", &cachedMem);
			if (strncmp(buffer, "SwapTotal:", strlen("SwapTotal:")) == 0)
				sscanf(buffer + strlen("SwapTotal:"), " %32llu kB", &totalSwap);
			if (strncmp(buffer, "SwapFree:", strlen("SwapFree:")) == 0)
				sscanf(buffer + strlen("SwapFree:"), " %32llu kB", &swapFree);
			if (strncmp(buffer, "SReclaimable:", strlen("SReclaimable:")) == 0)
				sscanf(buffer + strlen("SReclaimable:"), " %32llu kB", &sreclaimable);
			if (strncmp(buffer, "Shmem:", strlen("Shmem:")) == 0)
				sscanf(buffer + strlen("Shmem:"), " %32llu kB", &shmem);
			if (strncmp(buffer, "Slab:", strlen("Slab:")) == 0)
				sscanf(buffer + strlen("Slab:"), " %lu kB", &kb_slab);
		}
		fclose(file);

		//unsigned long kb_main_cached = cachedMem + kb_slab;
		//buffersMem + kb_main_cached is free command buff/cache
		cachedMem = cachedMem + sreclaimable - shmem;
		usedSwap = totalSwap - swapFree;
		usedMem = totalMem - freeMem - (buffersMem + cachedMem);  //htop
		//kb_main_used = kb_main_total - kb_main_free - kb_main_cached - kb_main_buffers;
		//buffersMem + cachedMem;

		minfo->totalMem = totalMem;
		minfo->usedMem = usedMem;
		minfo->freeMem = totalMem - usedMem;
	}
}

unsigned long os_clock_monotonic_ms()
{
	struct timespec times =
				{ 0, 0 };
	uint64_t time;
	clock_gettime(CLOCK_MONOTONIC, &times);
	//printf("CLOCK_MONOTONIC: %lu, %lu\n", times.tv_sec, times.tv_nsec);
//	if (1 == type)
//		time = times.tv_sec;
//	else
	time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	//printf("time = %ld\n", time);
	return time;
}

void os_pthread_set_name(const char *format, ...)
{
	char name[16];
	va_list v;
	va_start(v, format);
	vsnprintf(name, sizeof(name), format, v);
	va_end(v);
	prctl(PR_SET_NAME, name);
}

static char* concat_subpath_file(const char *path, const char *name)
{
	if (!path)
		path = "";
	char last = 0;
	int len = strlen(path);
	if (len > 0)
		len -= 1;
	last = path[len];
	while (*name == '/')
		name++;

	char *allp = 0;
	int size = 0;
	FILE *fp = open_memstream(&allp, &size);
	if (fp)
	{
		fprintf(fp, "%s%s%s", path, (last != '/' ? "/" : ""), name);
		fclose(fp);
	}
	return allp;
}

//文件的大小(byte)
unsigned long long os_path_size(const char *filename)
{
	unsigned long long sum = 0;
	struct stat statbuf;

	//操作的是软链接文件本身
	if (lstat(filename, &statbuf) != 0)
	{
		return 0;
	}

	//文件所占块数
	sum = statbuf.st_blocks * 512;

	//printf("[%s] statbuf.st_blksize=%d\n", filename, statbuf.st_blksize);
	if (S_ISLNK(statbuf.st_mode))
	{
	}
	if (S_ISDIR(statbuf.st_mode))
	{
		DIR *dir;
		struct dirent *entry;
		char *newfile;

		dir = opendir(filename);
		if (!dir)
			return sum;

		while ((entry = readdir(dir)))
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			newfile = concat_subpath_file(filename, entry->d_name);
			if (newfile == NULL)
				continue;
			sum += os_path_size(newfile);
			free(newfile);
		}
		closedir(dir);
	}
	return sum;
}

