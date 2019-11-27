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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

/* 计算校验和的算法 */
static unsigned short cal_chksum(unsigned short *addr, int len) {
	int sum = 0;
	int nleft = len;
	unsigned short *w = addr;
	unsigned short answer = 0;
	/* 把ICMP报头二进制数据以2字节为单位累加起来 */
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	/*
	 * 若ICMP报头为奇数个字节，会剩下最后一字节。
	 * 把最后一个字节视为一个2字节数据的高字节，
	 * 这2字节数据的低字节为0，继续累加
	 */
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(unsigned char *) w;
		sum += answer; /* 这里将 answer 转换成 int 整数 */
	}
	sum = (sum >> 16) + (sum & 0xffff); /* 高位低位相加 */
	sum += (sum >> 16); /* 上一步溢出时，将溢出位也加到sum中 */
	answer = ~sum; /* 注意类型转换，现在的校验和为16位 */
	return answer;
}

static int ping_getpack(void *sendbuf, int id, int seq) {
	struct icmp *icmp = NULL;
	int packsize = 0;
	//struct timeval *tval;

	icmp = (struct icmp*) sendbuf;
	icmp->icmp_type = ICMP_ECHO; /* icmp的类型 */
	icmp->icmp_code = 0; /* icmp的编码 */
	icmp->icmp_cksum = 0; /* icmp的校验和 */
	icmp->icmp_id = id; /* icmp的标志符 */
	icmp->icmp_seq = seq; /* icmp的顺序号 */

	packsize = 8 + 56; /* icmp8字节的头 加上数据的长度(datalen=56), packsize = 64 */
	//tval = (struct timeval *) icmp->icmp_data; /* 获得icmp结构中最后的数据部分的指针 */
	//gettimeofday(tval, NULL); /* 将发送的时间填入icmp结构中最后的数据部分 */
	strcpy((char*) icmp->icmp_data, "hello my name is cc.");
	icmp->icmp_cksum = cal_chksum((unsigned short *) icmp, packsize);/*填充发送方的校验和*/
	return packsize;
}

static int ping_check(in_addr_t saddr, const uint8_t *pack, int len) {
	struct iphdr *iphdr = NULL;
	struct icmp *icmp = NULL;
	int i = 0;
	char ips[20];

	iphdr = (struct iphdr*) pack;
	//icmp = (struct icmp*) (pack + sizeof(iphdr));
	icmp = (struct icmp*) (pack + (iphdr->ihl << 2));

	if (saddr == iphdr->saddr && icmp->icmp_type == ICMP_ECHOREPLY)
		return 1;

	//printf("src:%s ", inet_ntop(AF_INET, &iphdr->saddr, ips, sizeof(ips)));
	//printf("dst:%s ", inet_ntop(AF_INET, &iphdr->daddr, ips, sizeof(ips)));
	//printf("icmp_type=%d\n", icmp->icmp_type);
	return 0;
}

enum PingStat {
	PingStat_Start, //
	PingStat_Exit, //
	PingStat_Echo,	//
	PingStat_Host, //
	PingStat_UnknowHost,	//
	PingStat_TimeOut,	//
	PingStat_Error //
};

typedef void (*PingBackCall)(void *user, enum PingStat stat, const char *text);

struct PingAsync {
	char hostname[120];
	PingBackCall callback;
	void *user;
	int reqcount;
	int timeoutsec;
};

static void _PingAsyncCall(struct PingAsync *parg, enum PingStat stat,
		const char *text) {
	if (parg->callback) {
		parg->callback(parg->user, stat, text);
	}
}

static int ping(struct PingAsync *parg) {
	//ping
	struct hostent *hp;
	struct sockaddr_in whereto; /* who to ping */
	struct sockaddr_in fromaddr;
	int icmp_sock;
	int ret;
	int icmp_echo_count = 0;

	_PingAsyncCall(parg, PingStat_Start, "");

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock == -1) {
		perror("socket create err..");
		ret = -1;
		goto _retexit;
	}

	memset(&whereto, 0, sizeof(whereto));
	whereto.sin_family = AF_INET;
	if (inet_aton(parg->hostname, &whereto.sin_addr) == 1) {

	} else {
		hp = gethostbyname2(parg->hostname, AF_INET);
		if (hp) {
			memcpy(&whereto.sin_addr, hp->h_addr, 4);
		}
		else
		{
			_PingAsyncCall(parg,PingStat_UnknowHost, "");
			ret=-1;
			goto _retexit;
		}
	}

	int size = 10 * 1024;
	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	char sendpacket[10 * 1024];
	uint8_t recvpacket[10 * 1024];

	int packsize;
	int i = 0;

	memset(sendpacket, 0, 30);
	inet_ntop(AF_INET, &whereto.sin_addr, sendpacket, 30);
	_PingAsyncCall(parg, PingStat_Host, sendpacket);

	fcntl(icmp_sock, F_SETFL, O_NONBLOCK);

	{
		struct timeval timeo;
		struct timeval timestart, timeend;
		fd_set set;

		while (i < 5) {
			memset(sendpacket, 0, sizeof(sendpacket));
			packsize = ping_getpack(sendpacket, i++, 1);
			timeo.tv_sec = 3;
			timeo.tv_usec = 0;

			gettimeofday(&timestart, NULL);

			sendto(icmp_sock, sendpacket, packsize, 0,
					(struct sockaddr *) &whereto, sizeof(whereto));

			FD_ZERO(&set);
			FD_SET(icmp_sock, &set);

			ret = select(icmp_sock + 1, &set, NULL, NULL, &timeo);
			if (ret == -1) {
				_PingAsyncCall(parg, PingStat_Error, "icmp select error");
			} else if (ret == 0) {
				_PingAsyncCall(parg, PingStat_TimeOut, "");
			} else {
				if (FD_ISSET(icmp_sock, &set)) {
					socklen_t len;
					int ret = 0;
					len = sizeof(fromaddr);

					memset(recvpacket, 0, sizeof(recvpacket));

					ret = recvfrom(icmp_sock, recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *) &fromaddr, &len);
					gettimeofday(&timeend, NULL);

					if (ping_check(whereto.sin_addr.s_addr, recvpacket, ret)) {
						float sumtm = timeend.tv_sec - timestart.tv_sec;

						sumtm *= 1000;
						sumtm += (timeend.tv_usec - timestart.tv_usec);

						sprintf(sendpacket, "time=%.2fms", sumtm/1000);
						_PingAsyncCall(parg, PingStat_Echo, sendpacket);
						icmp_echo_count++;
					}
//					else {
//						_PingAsyncCall(parg, PingStat_Error, "not the ip echo");
//					}
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

static void *_Run(void *arg) {
	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "Ping");

	struct PingAsync *parg = (struct PingAsync*) arg;
	int ret;
	ret = ping(parg);
	free(parg);
	return 0;
}

int ToolPingAsync(const char *host, void *bindData, PingBackCall funbk) {
	pthread_t pt;
	int ret;
	struct PingAsync *arg = (struct PingAsync*) malloc(
			sizeof(struct PingAsync));
	if (!arg)
		return -1;

	arg->callback = funbk;
	arg->user = bindData;
	strncpy(arg->hostname, host, sizeof(arg->hostname));
	arg->reqcount = 5;
	arg->timeoutsec = 3;
	ret = pthread_create(&pt, NULL, _Run, arg);
	if (ret) {
		free(arg);
	}
	return ret;
}

static void _pingBackCall(void *user, enum PingStat stat, const char *text) {
	printf("%d %s\n", stat, text);
}

//-1 错误
//0 不存在或超时
//>1 respcount
int ToolPingSync(const char *host, int reqcount, int timeoutsec) {
	//ping
	struct PingAsync arg;
	int ret;
	memset(&arg, 0, sizeof(arg));

	strncpy(arg.hostname, host, sizeof(arg.hostname));
	arg.callback = _pingBackCall;
	arg.user = NULL;
	arg.reqcount = reqcount;
	arg.timeoutsec = timeoutsec;
	ret= ping(&arg);
	return ret;
}

//	struct hostent *hp;
//	struct sockaddr_in whereto; /* 目标地址 */
//	struct sockaddr_in fromaddr; /*接收到数据的地址*/
//	int icmp_sock;
//	int icmp_echo_count = 0;
//
//	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
//	if (icmp_sock == -1) {
//		perror("socket create err..");
//		return -1;
//	}
//
//	memset(&whereto, 0, sizeof(whereto));
//	whereto.sin_family = AF_INET;
//	if (inet_aton(host, &whereto.sin_addr) == 1) {
//
//	} else {
//		hp = gethostbyname2(host, AF_INET);
//		if (hp) {
//			memcpy(&whereto.sin_addr, hp->h_addr, 4);
//		}
//		else
//		{
//			perror("not get host ip");
//			close(icmp_sock);
//			return -1;
//		}
//	}
//
//	//接收buf
//	int size = 10 * 1024;
//	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
//
//	char sendpacket[10 * 1024];
//	uint8_t recvpacket[10 * 1024];
//
//	int packsize;
//	int i = 0;
//
//	printf("PING %s start\n", inet_ntoa(whereto.sin_addr));
//
//	//非阻塞
//	fcntl(icmp_sock, F_SETFL, O_NONBLOCK);
//
//	{
//		struct timeval timeo;
//		struct timeval timestart, timeend;
//		fd_set set;
//
//		int ret;
//		while (i < reqcount) {
//			memset(sendpacket, 0, sizeof(sendpacket));
//			packsize = ping_getpack(sendpacket, i++, 1);
//			printf("index:%d", i);
//
//			timeo.tv_sec = timeoutsec;
//			timeo.tv_usec = 0;
//
//			gettimeofday(&timestart, NULL);
//			sendto(icmp_sock, sendpacket, packsize, 0,
//					(struct sockaddr *) &whereto, sizeof(whereto));
//
//			FD_ZERO(&set);
//			FD_SET(icmp_sock, &set);
//
//			ret = select(icmp_sock + 1, &set, NULL, NULL, &timeo);
//			if (ret == -1) {
//				printf(" icmp select error");
//			} else if (ret == 0) {
//				printf(" icmp timeout\n");
//			} else {
//				if (FD_ISSET(icmp_sock, &set)) {
//					socklen_t len;
//					int ret = 0;
//					len = sizeof(fromaddr);
//
//					memset(recvpacket, 0, sizeof(recvpacket));
//
//					ret = recvfrom(icmp_sock, recvpacket, sizeof recvpacket, 0,
//							(struct sockaddr *) &fromaddr, &len);
//					gettimeofday(&timeend, NULL);
//
//					if (ping_check(whereto.sin_addr.s_addr, recvpacket, ret)) {
//						icmp_echo_count++;
//						printf(" %s live:%ldms\n", inet_ntoa(whereto.sin_addr),
//								(timeend.tv_sec * 1000 + timeend.tv_usec / 1000)
//										- (timestart.tv_sec * 1000
//												+ timestart.tv_usec / 1000));
//					} else {
//						printf("icmp err:ret=%d", ret);
//					}
//				}
//			}
//		}
//	}
//	close(icmp_sock);

