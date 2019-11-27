/*
 * ping.c
 *
 *  Created on: 2019年4月30日
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
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>

#define __USE_XOPEN2K
#define __USE_GNU
#define __USE_MISC 1

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <pthread.h>

#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define BB_LITTLE_ENDIAN 1

uint16_t inet_cksum(void *paddr, int nleft)
{
	/*
   * Our algorithm is simple, using a 32 bit accumulator,
   * we add sequential 16 bit words to it, and at the end, fold
   * back all the carry bits from the top 16 bits into the lower
   * 16 bits.
   */
	uint16_t *addr = (uint16_t *)paddr;
	unsigned sum = 0;
	while (nleft > 1)
	{
		sum += *addr++;
		nleft -= 2;
	}

	/* Mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		if (BB_LITTLE_ENDIAN)
			sum += *(uint8_t *)addr;
		else
			sum += *(uint8_t *)addr << 8;
	}

	/* Add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16);					/* add carry */

	return (uint16_t)~sum;
}

/* 计算校验和的算法 */
static unsigned short cal_chksum(unsigned short *addr, int len)
{
	int sum = 0;
	int nleft = len;
	unsigned short *w = addr;
	unsigned short answer = 0;
	/* 把ICMP报头二进制数据以2字节为单位累加起来 */
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	/*
   * 若ICMP报头为奇数个字节，会剩下最后一字节。
   * 把最后一个字节视为一个2字节数据的高字节，
   * 这2字节数据的低字节为0，继续累加
   */
	if (nleft == 1)
	{
		*(unsigned char *)(&answer) = *(unsigned char *)w;
		sum += answer; /* 这里将 answer 转换成 int 整数 */
	}
	sum = (sum >> 16) + (sum & 0xffff); /* 高位低位相加 */
	sum += (sum >> 16);					/* 上一步溢出时，将溢出位也加到sum中 */
	answer = ~sum;						/* 注意类型转换，现在的校验和为16位 */
	return answer;
}

static int ping_getpack(void *sendbuf, uint16_t id, uint16_t seq)
{
	struct icmp *icmp = NULL;
	int packsize = 0;
	// struct timeval *tval;

	icmp = (struct icmp *)sendbuf;
	icmp->icmp_type = ICMP_ECHO; /* icmp的类型 */
	icmp->icmp_code = 0;		 /* icmp的编码 */
	icmp->icmp_cksum = 0;		 /* icmp的校验和 */
	icmp->icmp_id = id;			 /* icmp的标志符 */
	icmp->icmp_seq = seq;		 /* icmp的顺序号 */

	/* icmp8字节的头 加上数据的长度(datalen=56), packsize = 64 */
	packsize = 8 + 56;
	// tval = (struct timeval *) icmp->icmp_data; /*
	// 获得icmp结构中最后的数据部分的指针 */ gettimeofday(tval, NULL); /*
	// 将发送的时间填入icmp结构中最后的数据部分 */
	strcpy((char *)icmp->icmp_data, "hello my name is cc.");
	icmp->icmp_cksum =
		cal_chksum((unsigned short *)icmp, packsize); /*填充发送方的校验和*/
	return packsize;
}

static int ping_check(in_addr_t saddr, const uint8_t *pack, int len)
{
	struct iphdr *iphdr = NULL;
	struct icmp *icmp = NULL;
	int i = 0;
	char ips[20];

	iphdr = (struct iphdr *)pack;
	// icmp = (struct icmp*) (pack + sizeof(iphdr));
	icmp = (struct icmp *)(pack + (iphdr->ihl << 2));

	if (saddr == iphdr->saddr && icmp->icmp_type == ICMP_ECHOREPLY)
	{
		uint16_t recv_id = icmp->icmp_id;
		uint16_t recv_seq = ntohs(icmp->icmp_seq);
		uint8_t ttl = iphdr->ttl;

		printf("src:%s ", inet_ntop(AF_INET, &iphdr->saddr, ips, sizeof(ips)));
		return 1;
	}
	if(icmp->icmp_type==ICMP_DEST_UNREACH)
	{ //不存在

	}
	// printf(">src:%s ", inet_ntop(AF_INET, &iphdr->saddr, ips, sizeof(ips)));
	// printf("dst:%s ", inet_ntop(AF_INET, &iphdr->daddr, ips, sizeof(ips)));
	//printf("icmp_type=%d\n", icmp->icmp_type);
	return 0;
}

enum PingStat
{
	PingStat_Start,		 //
	PingStat_Exit,		 //
	PingStat_Echo,		 //
	PingStat_Host,		 //
	PingStat_UnknowHost, //
	PingStat_TimeOut,	//
	PingStat_Error		 //
};

typedef void (*PingBackCall)(void *user, enum PingStat stat, const char *text);

struct PingAsync
{
	char hostname[120];
	PingBackCall callback;
	void *user;
	int reqcount;
	int timeoutsec;
};

static void _PingAsyncCall(struct PingAsync *parg, enum PingStat stat,
						   const char *text)
{
	if (parg->callback)
	{
		parg->callback(parg->user, stat, text);
	}
}

static int ping(struct PingAsync *parg)
{
	// ping
	struct hostent *hp;
	struct sockaddr_in whereto; /* who to ping */
	struct sockaddr_in fromaddr;
	int icmp_sock = -1;
	int ret;
	int icmp_echo_count = 0;

	_PingAsyncCall(parg, PingStat_Start, "");

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock < 0)
	{
		icmp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	}
	if (icmp_sock == -1)
	{
		perror("socket create err..");
		ret = -1;
		goto _retexit;
	}

	memset(&whereto, 0, sizeof(whereto));
	whereto.sin_family = AF_INET;
	if (inet_aton(parg->hostname, &whereto.sin_addr) == 1)
	{
	}
	else
	{
		hp = gethostbyname2(parg->hostname, AF_INET);
		if (hp)
		{
			memcpy(&whereto.sin_addr, hp->h_addr, 4);
		}
		else
		{
			_PingAsyncCall(parg, PingStat_UnknowHost, "");
			ret = -1;
			goto _retexit;
		}
	}
	{
		int size = (56 * 2) + 7 * 1024;
		setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	}

	char sendpacket[1024];
	uint8_t recvpacket[1024];

	int packsize;

	memset(sendpacket, 0, 30);
	inet_ntop(AF_INET, &whereto.sin_addr, sendpacket, 30);
	_PingAsyncCall(parg, PingStat_Host, sendpacket);

	fcntl(icmp_sock, F_SETFL, O_NONBLOCK);

	{
		struct timeval timeo;
		struct timeval timestart, timeend;
		fd_set set;
		uint16_t _icmp_id = time(NULL)+rand();//getpid()+rand()%100;
		int _icmp_seq = 0;
		int need_send_flag = 1;
		while (_icmp_seq < parg->reqcount)
		{
			//memset(sendpacket, 0, sizeof(sendpacket));
			if (need_send_flag)
			{
				gettimeofday(&timestart, NULL);
				packsize = ping_getpack(sendpacket, _icmp_id, ntohs(_icmp_seq));
				sendto(icmp_sock, sendpacket, packsize, 0, (struct sockaddr *)&whereto,
					   sizeof(whereto));
				_icmp_seq++;
				need_send_flag = 0;
			}

			FD_ZERO(&set);
			FD_SET(icmp_sock, &set);
			timeo.tv_sec = 3;
			timeo.tv_usec = 0;
			ret = select(icmp_sock + 1, &set, NULL, NULL, &timeo);
			if (ret == -1)
			{
				_PingAsyncCall(parg, PingStat_Error, "icmp select error");
				need_send_flag = 1;
			}
			else if (ret == 0)
			{
				_PingAsyncCall(parg, PingStat_TimeOut, "");
				need_send_flag = 1;
			}
			else
			{
				if (FD_ISSET(icmp_sock, &set))
				{
					socklen_t len;
					int ret = 0;
					len = sizeof(fromaddr);

					memset(recvpacket, 0, sizeof(recvpacket));

					ret = recvfrom(icmp_sock, recvpacket, sizeof recvpacket, 0,
								   (struct sockaddr *)&fromaddr, &len);

					{
						struct icmp *icmppkt;
						struct iphdr *iphdr;
						int hlen;
						iphdr = (struct iphdr *)recvpacket;
						hlen = iphdr->ihl << 2;

						icmppkt = (struct icmp *)(recvpacket + hlen);
						if (icmppkt->icmp_id != _icmp_id &&
							!ping_check(whereto.sin_addr.s_addr, recvpacket, ret))
						{
							//printf("need restart recv..\n");
							continue;
						}

						{
							gettimeofday(&timeend, NULL);
							//1s=1000ms=1000_0000us

							double t2 = timeend.tv_sec * 10000 + timeend.tv_usec / 100;
							double t1 = timestart.tv_sec * 10000 + timestart.tv_usec / 100;

							sprintf(sendpacket, "time=%.1fms", (t2 - t1) / 10);
							_PingAsyncCall(parg, PingStat_Echo, sendpacket);
							icmp_echo_count++;
							need_send_flag = 1;
						}
					}
				}
			}
		} // end while
	}

	ret = icmp_echo_count;

_retexit:

	if (icmp_sock != -1)
		close(icmp_sock);

	sprintf(sendpacket, "recv=%d", icmp_echo_count);
	_PingAsyncCall(parg, PingStat_Exit, sendpacket);
	return ret;
}

static void *_Run(void *arg)
{
	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "Ping");

	struct PingAsync *parg = (struct PingAsync *)arg;
	int ret;
	ret = ping(parg);
	free(parg);
	return 0;
}

int ToolPingAsync(const char *host, void *bindData, PingBackCall funbk)
{
	pthread_t pt;
	int ret;
	struct PingAsync *arg = (struct PingAsync *)malloc(sizeof(struct PingAsync));
	if (!arg)
		return -1;

	arg->callback = funbk;
	arg->user = bindData;
	strncpy(arg->hostname, host, sizeof(arg->hostname));
	arg->reqcount = 5;
	arg->timeoutsec = 3;
	ret = pthread_create(&pt, NULL, _Run, arg);
	if (ret)
	{
		free(arg);
	}
	return ret;
}

static void _pingBackCall(void *user, enum PingStat stat, const char *text)
{
	printf("%d %s\n", stat, text);
}

//-1 错误
// 0 不存在或超时
//>1 respcount
int ToolPingSync(const char *host, int reqcount, int timeoutsec)
{
	// ping
	struct PingAsync arg;
	int ret;
	memset(&arg, 0, sizeof(arg));

	strncpy(arg.hostname, host, sizeof(arg.hostname));
	arg.callback = _pingBackCall;
	arg.user = NULL;
	arg.reqcount = reqcount;
	arg.timeoutsec = timeoutsec;
	ret = ping(&arg);
	return ret;
}

#define CONFIG_TEST_DEBUG

#ifdef CONFIG_TEST_DEBUG

int main(int argc, char const *argv[])
{
	char *host = "www.baidu.com";

	struct addrinfo *result = NULL;
	struct addrinfo hint;

	memset(&hint, 0, sizeof(struct addrinfo));

	//int ai_flags=DIE_ON_ERROR;
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	// hint.ai_flags = AI_PASSIVE;
	// //hint.ai_flags = ai_flags & ~DIE_ON_ERROR;
	// hint.ai_family = AF_INET;   /* Allow IPv4 */
	// hint.ai_flags = AI_PASSIVE; /* For wildcard IP address */
	// hint.ai_protocol = 0;		 /* Any protocol */
	// hint.ai_socktype = SOCK_STREAM;

	int rc = getaddrinfo(host, NULL, &hint, &result);
	if (!rc)
	{
		struct addrinfo *used_res;
		used_res = result;

		while (1)
		{
			if (used_res->ai_family == AF_INET)
				break;

			used_res = used_res->ai_next;
			if (!used_res)
			{
				used_res = result;
				break;
			}
		}
		//struct sockaddr
		if (used_res)
		{
			struct sockaddr_in *addr = (struct sockaddr_in *)used_res->ai_addr;
			printf("%s\n", inet_ntoa(addr->sin_addr));
		}

		freeaddrinfo(result);
	}

	ToolPingSync(argv[1], 5, 2);
	return 0;
}
#endif