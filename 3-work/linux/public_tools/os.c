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
		snprintf(buffer, sizeof(buffer), "%d", htons(local_port));

		if ((strlen(local_addr) + strlen(buffer)) > 22)
			local_addr[22 - strlen(buffer)] = '\0';

		strcat(local_addr, ":");
		strcat(local_addr, buffer);
		snprintf(buffer, sizeof(buffer), "%d", htons(rem_port));

		if ((strlen(rem_addr) + strlen(buffer)) > 22)
			rem_addr[22 - strlen(buffer)] = '\0';

		strcat(rem_addr, ":");
		strcat(rem_addr, buffer);
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
		strncpy(pitem->foreign_addr, rem_addr, 30);
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
		snprintf(buffer, sizeof(buffer), "%d", htons(local_port));
		if ((strlen(local_addr) + strlen(buffer)) > 22)
			local_addr[22 - strlen(buffer)] = '\0';
		strcat(local_addr, ":");
		strcat(local_addr, buffer);

		snprintf(buffer, sizeof(buffer), "%d", htons(rem_port));
		inet_ntop(AF_INET, &remaddr.sin_addr.s_addr, rem_addr, sizeof(rem_addr));
		//safe_strncpy(rem_addr, ap->sprint((struct sockaddr *) &remaddr,	flag_not), sizeof(rem_addr));
		if ((strlen(rem_addr) + strlen(buffer)) > 22)
			rem_addr[22 - strlen(buffer)] = '\0';
		strcat(rem_addr, ":");
		strcat(rem_addr, buffer);

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
		strncpy(pitem->foreign_addr, rem_addr, 30);
		strncpy(pitem->state, tcp_state[state], 15);
		pitem->inode = inode;
		strncpy(pitem->timers, timers, 64);
	}
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
