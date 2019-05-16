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

/*数据类型别名*/
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

static volatile int _flagRun = 0;
static char _address[30];
static void *_bindData = NULL;
static void (*_funbk)(void *bindData, const char *msg) = NULL;

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
	icmp = (struct icmp*) (pack + (iphdr->ihl<< 2));

	if(saddr==iphdr->saddr && icmp->icmp_type == ICMP_ECHOREPLY)
		return 1;

	//printf("src:%s ", inet_ntop(AF_INET, &iphdr->saddr, ips, sizeof(ips)));
	//printf("dst:%s ", inet_ntop(AF_INET, &iphdr->daddr, ips, sizeof(ips)));
	//printf("icmp_type=%d\n", icmp->icmp_type);
	return 0;
}

static void backmsg(const char *format, ...) {
	char buff[300];
	va_list ap;
	va_start(ap, format);
	vsprintf(buff, format, ap);
	va_end(ap);

	_funbk(_bindData, buff);
}

static void ping(const char *address) {
//ping
	struct hostent *hp;
	struct sockaddr_in whereto; /* who to ping */
	struct sockaddr_in fromaddr;
	int icmp_sock;

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock == -1) {
		perror("socket create err..");
		return;
	}

	memset(&whereto, 0, sizeof(whereto));
	whereto.sin_family = AF_INET;
	if (inet_aton(address, &whereto.sin_addr) == 1) {

	} else {
		hp = gethostbyname2(address, AF_INET);
		if (hp) {
			memcpy(&whereto.sin_addr, hp->h_addr, 4);
		}
		else
		{
			backmsg("not gethostbyname2 %s", address);
			return;
		}
	}

	int size = 10 * 1024;
	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

	char sendpacket[10 * 1024];
	uint8_t recvpacket[10 * 1024];

	int packsize;
	int i = 0;

	backmsg("ping %s", inet_ntoa(whereto.sin_addr));

	fcntl(icmp_sock, F_SETFL, O_NONBLOCK);

	{
		struct timeval timeo;
		struct timeval timestart, timeend;
		fd_set set;

		int ret;
		while (i < 5) {
			memset(sendpacket, 0, sizeof(sendpacket));
			packsize = ping_getpack(sendpacket, i++, 1);

			backmsg("icmp-id:%d", i);
			timeo.tv_sec = 3;
			timeo.tv_usec = 0;

			gettimeofday(&timestart, NULL);

			sendto(icmp_sock, sendpacket, packsize, 0,
					(struct sockaddr *) &whereto, sizeof(whereto));

			FD_ZERO(&set);
			FD_SET(icmp_sock, &set);

			ret = select(icmp_sock + 1, &set, NULL, NULL, &timeo);
			if (ret == -1) {
				backmsg("icmp select error");
			} else if (ret == 0) {
				backmsg("icmp timeout");
			} else {
				if (FD_ISSET(icmp_sock, &set)) {
					socklen_t len;
					int ret = 0;
					len = sizeof(fromaddr);

					memset(recvpacket, 0, sizeof(recvpacket));

					ret = recvfrom(icmp_sock, recvpacket, sizeof recvpacket, 0,
							(struct sockaddr *) &fromaddr, &len);
					gettimeofday(&timeend, NULL);

					if (ping_check(whereto.sin_addr.s_addr,recvpacket, ret)) {
						backmsg("live:%ldms",
								(timeend.tv_sec * 1000 + timeend.tv_usec / 1000)
										- (timestart.tv_sec * 1000
												+ timestart.tv_usec / 1000));
					} else {
						backmsg("icmp err:ret=%d", ret);
					}
				}
			}
		}
	}
	close(icmp_sock);
}

static void _loop() {
	ping(_address);
}

static void *_Run(void *arg) {
	pthread_detach(pthread_self());
	prctl(PR_SET_NAME, "Ping");
	backmsg("start");
	_loop();
	backmsg("end");
	_flagRun = 0;
	return 0;
}

int ToolPing(const char *ip, void *bindData,
		void (*funbk)(void *bindData, const char *msg)) {
	pthread_t pt;
	if (_flagRun)
		return -1;

	_flagRun = 1;
	_funbk = funbk;
	_bindData = bindData;
	memset(_address, 0, sizeof(_address));
	strncpy(_address, ip, sizeof(_address) - 1);
	pthread_create(&pt, NULL, _Run, NULL);
	return 0;
}


void myprint(void *ctx, const char *msg) {
	printf("%s\n", msg);
}

int main(int argc, char **argv) {
	_bindData = myprint;
	ping("www.baidu.com");
	return 0;
}

