/*
 * main.c
 *　这是测试iwlist的命令操作，使用ioctl函数
 *　可以扫描wifi列表,并不需要iwlist scanning 命令操作
 *  Created on: 2018年4月12日
 *      Author: cj
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#include <sys/socket.h>
//#include <linux/wireless.h>
//#include <sys/ioctl.h>

#include "iwtools.h"
#include "errno.h"
#include "iwlib.h"

typedef struct iwscan_state {
	/* State */
	int ap_num; /* Access Point number 1->N */
	int val_index; /* Value in table 0->(N-1) */
} iwscan_state;

struct ap_point {
	struct ap_point *next;
	struct sockaddr ap_addr; /* Access point address */
	char essid[IW_ESSID_MAX_SIZE + 1];
	int mode;
	char protocol[IFNAMSIZ + 1];
};

static inline void print_scanning_token(struct stream_descr * stream, /* Stream of events */
struct iw_event * event, /* Extracted token */
struct iwscan_state * state, struct iw_range * iw_range, /* Range info */
int has_range) {

	char buffer[128]; /* Temporary buffer */

	/* Now, let's decode the event */
	switch (event->cmd) {
	case SIOCGIWAP:
		printf("          Cell %02d - Address: %s\n", state->ap_num,
				iw_saether_ntop(&event->u.ap_addr, buffer));
		state->ap_num++;
		break;
	case SIOCGIWESSID: {
		char essid[IW_ESSID_MAX_SIZE + 1];
		memset(essid, '\0', sizeof(essid));
		if ((event->u.essid.pointer) && (event->u.essid.length))
			memcpy(essid, event->u.essid.pointer, event->u.essid.length);
		if (event->u.essid.flags) {
			/* Does it have an ESSID index ? */
			if ((event->u.essid.flags & IW_ENCODE_INDEX) > 1)
				printf("                    ESSID:\"%s\" [%d]\n", essid,
						(event->u.essid.flags & IW_ENCODE_INDEX));
			else
				printf("                    ESSID:\"%s\"\n", essid);
		} else
			printf("                    ESSID:off/any/hidden\n");
	}
		break;
	case SIOCGIWNWID:
		if (event->u.nwid.disabled)
			printf("                    NWID:off/any\n");
		else
			printf("                    NWID:%X\n", event->u.nwid.value);
		break;

	}
}

int iwlist_scanning(const char *ifname, struct ap_point **aplist, int *ap_size) {
	int skfd = iwpriv_sockets_open();
	char buffer[4096 * 3];  //IW_SCAN_MAX_DATA*3
	struct iwreq wrq;
	int ret = 0;
	struct timeval tv; /* Select timeout */
	struct ap_point *apalls = NULL;

	tv.tv_sec = 0;
	tv.tv_usec = 250000;
	strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(skfd, SIOCSIWSCAN, &wrq);
	if (ret < 0) {
		if (errno != EPERM) {
			printf("SIOCSIWSCAN init err...:%d, %s\n", errno, strerror(errno));
			goto _err;
		}
		tv.tv_usec = 0;
	}

	while (1) {
		fd_set rfds; /* File descriptors for select */
		int last_fd; /* Last fd */
		int ret;

		tv.tv_usec = 0;
		/* Guess what ? We must re-generate rfds each time */
		FD_ZERO(&rfds);
		last_fd = -1;

		ret = select(last_fd + 1, &rfds, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			fprintf(stderr, "Unhandled signal - exiting...\n");
			goto _err;
		}
		if (ret == 0) {
			memset(buffer, 0, sizeof(buffer));
			wrq.u.data.pointer = buffer;
			wrq.u.data.flags = 0;
			wrq.u.data.length = sizeof(buffer);
			ret = ioctl(skfd, SIOCGIWSCAN, &wrq);
			if (ret == 0) {
				break;
			}
			if (errno == E2BIG) {
				printf("SIOCGIWSCAN init err...:%d, %s\n", errno, strerror(errno));
			}
			goto _err;
			//printf("wrq.u.data.length =%d", wrq.u.data.length);
		}
	}

	if (wrq.u.data.length <= 0) {
		close(skfd);
		*ap_size = 0;
		return 0;
	}

	char *current = NULL, *end = NULL;
	struct iw_event iwe;
	int ap_index = -1;
	current = buffer;
	end = buffer + wrq.u.data.length;
	*ap_size = 0;

	while (1) {
		if ((current + IW_EV_LCP_PK_LEN) > end)
			break;
		memcpy(&iwe, current, IW_EV_LCP_PK_LEN);
		if (iwe.cmd == SIOCGIWAP)
			(*ap_size)++;
		current += iwe.len;
	}

	current = buffer;
	end = buffer + wrq.u.data.length;

	if (*ap_size > 0) {
		apalls = (struct ap_point *) malloc(sizeof(struct ap_point) * *ap_size);
		*aplist = apalls;
		memset(apalls, 0, sizeof(struct ap_point) * *ap_size);

		do {
			//ret = iw_extract_event_stream(&stream, &iwe, range.we_version_compiled);
			//if (ret > 0)
			//	print_scanning_token(&stream, &iwe, &state, &range, has_range);

			if ((current + IW_EV_LCP_PK_LEN) > end)
				break;
			/* Extract an event and print it */
			memcpy(&iwe, current, IW_EV_LCP_PK_LEN);

			switch (iwe.cmd) {  //iwe_stream_add_event, 参考 kernel src: /include/net/iw_handler.h
			case SIOCGIWAP: {
				ap_index++;
				memcpy(&iwe.u, current + 4, sizeof(iwe.u));
				memcpy(&apalls[ap_index].ap_addr, &iwe.u.ap_addr, sizeof(struct sockaddr));
			}
				break;
			case SIOCGIWESSID: { //POINT
				char essid[IW_ESSID_MAX_SIZE + 1];
				memset(essid, 0, sizeof(essid));
				memcpy(&iwe.u.data.length, current + 4, sizeof(iwe.u.data));
				iwe.u.data.pointer = current + 4 + sizeof(iwe.u.data.length)
						+ sizeof(iwe.u.data.flags);
				strncpy(essid, iwe.u.data.pointer, iwe.u.data.length);
				strcpy(apalls[ap_index].essid, essid);
				//printf("\tESSID:[%s]\n", essid);
			}
				break;
			case SIOCGIWMODE:  //iwe_stream_add_event
				memcpy(&iwe.u, current + 4, sizeof(iwe.u));
				if (iwe.u.mode >= IW_NUM_OPER_MODE)
					iwe.u.mode = IW_NUM_OPER_MODE;
				apalls[ap_index].mode = iwe.u.mode;
				break;
			case SIOCGIWNAME:
				memcpy(&iwe.u, current + 4, sizeof(iwe.u));
				strncpy(apalls[ap_index].protocol, iwe.u.name, iwe.len - 4);
				break;
			}
			current += iwe.len;
		} while (1);
	}
	close(skfd);
	return 0;

	_err: close(skfd);
	return -1;
}

int main(int argc, char **argv) {

	struct ap_point *aplist = NULL;
	int ap_size = 0;
	int i = 0;

	while (1) {
		printf("\033[2J");
		printf("\033[0;0H");
		iwlist_scanning("wlan0", &aplist, &ap_size);
		for (i = 0; i < ap_size; i++) {
			printf("AP %02d, family:%d ", i + 1, aplist[i].ap_addr.sa_family);
			printf("%02X:%02X:%02X:%02X:%02X:%02X", ((unsigned char*) aplist[i].ap_addr.sa_data)[0],
					((unsigned char*) aplist[i].ap_addr.sa_data)[1],
					((unsigned char*) aplist[i].ap_addr.sa_data)[2],
					((unsigned char*) aplist[i].ap_addr.sa_data)[3],
					((unsigned char*) aplist[i].ap_addr.sa_data)[4],
					((unsigned char*) aplist[i].ap_addr.sa_data)[5]);

			printf("\tESSID:[%s]", aplist[i].essid);
			printf("\tMode:%s", iw_operation_mode[aplist[i].mode]);
			printf("                    Protocol:%-1.16s\n", aplist[i].protocol);
		}
		if (aplist)
			free(aplist);
		sleep(1);
	}

	return 0;
}
