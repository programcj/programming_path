#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "wireless.h"

int iw_sockets_open(void) {
	static const int families[] = {
	AF_INET, AF_IPX, AF_AX25, AF_APPLETALK };
	unsigned int i;
	int sock;

	for (i = 0; i < sizeof(families) / sizeof(int); ++i) {
		/* Try to open the socket, if success returns it */
		sock = socket(families[i], SOCK_DGRAM, 0);
		if (sock >= 0)
			return sock;
	}
	return -1;
}

int iwpriv_command(const char *name, int iocmd, char *buff, int bufflen) {
	int skfd = iw_sockets_open();
	struct iwreq wrq;

	if (skfd == -1)
		return -1;
	wrq.u.data.pointer = buff;
	wrq.u.data.length = bufflen;
	wrq.u.data.flags = 0;
	strncpy(wrq.ifr_name, name, IFNAMSIZ);
	ioctl(skfd, iocmd, &wrq);
	close(skfd);
	return 0;
}

#define iwpriv_set(name, buff, bufflen) 	iwpriv_command(name, 0x8BE2, buff, bufflen)
#define iwpriv_get_site_survey(name, buff, bufflen) 	iwpriv_command(name, 0x8BED, buff, bufflen)
#define iwpriv_elian(name, buff, bufflen) 	iwpriv_command(name, 0x8BFB, buff, bufflen)
#define iwpriv_conn_status(name, buff, bufflen) 	iwpriv_command(name, 0x8BFC, buff, bufflen)

struct wifi_info {
	int ch;
	char ssid[33];
	char bssid[20];
	char security[23];
	char signal[9];
	char wmode[7];
	char extch[7];
	char nt[3];
	char wps[3];
};

static void string_delend_space(char *str) {
	char *ptr = str;
	int len = strlen(str);
	if (len == 0)
		return;
	ptr = str + len - 1;
	do {
		if (*ptr == ' ')
			*ptr = 0;
		else
			break;
		ptr--;
	} while (ptr > str);
}

struct wifi_info *wifi_list(const char *netname, int *size) {
	static struct wifi_info wifi_list[35];
	char buffer[10240];
	char *tmp = NULL;
	char *line = NULL;

	struct wifi_info wifi;
	int i = 0;

	memset(&wifi_list, 0, sizeof(wifi_list));
	*size = i;

	memset(buffer, 0, sizeof(buffer)); //iwpriv $name set AutoChannelSel=1
	strcpy(buffer, "AutoChannelSel=1");
	if (iwpriv_set(netname, buffer, sizeof(buffer)) != 0)
		return wifi_list;
	sleep(1);

	memset(buffer, 0, sizeof(buffer));  //iwpriv apcli0 get_site_survey
	if (iwpriv_get_site_survey(netname, buffer, sizeof(buffer)) != 0)
		return wifi_list;

	//#define	LINE_LEN	(4+33+20+23+9+7+7+3)	/* Channel+SSID+Bssid+Security+Signal+WiressMode+ExtCh+NetworkType*/
	line = strtok_r(buffer, "\n", &tmp);
	while (line != NULL) {
		memset(&wifi, 0, sizeof(struct wifi_info));
		if (strlen(line) > 5 && strncasecmp(line, "Ch", 2) != 0) {
			line[3] = 0;
			wifi.ch = atoi(line);
			line += 4;
			strncpy(wifi.ssid, line, 32);
			string_delend_space(wifi.ssid);
			line += 33;
			strncpy(wifi.bssid, line, 19);
			string_delend_space(wifi.bssid);
			line += 20;
			strncpy(wifi.security, line, 22);
			string_delend_space(wifi.security);
			line += 23;
			strncpy(wifi.signal, line, 9);
			string_delend_space(wifi.signal);
			line += 9;

			memcpy(&wifi_list[i], &wifi, sizeof(wifi));
			i++;
		}
		line = strtok_r(NULL, "\n", &tmp);
	}
	*size = i;
	return wifi_list;
}

/**
 * openwrt wifi connect program
 *
 */
int main(int argc, char **argv) {
	int size = 0;
	struct wifi_info *wifis = NULL;
	int i = 0;
	wifis = wifi_list("apcli0", &size);

	for (i = 0; i < size; i++) {
		printf("[%d][%s][%s][%s][%s]\n", wifis[i].ch, wifis[i].ssid, wifis[i].bssid,
				wifis[i].security, wifis[i].signal);
	}
	//debug_test();
	return 0;
}

void debug_test() {
	/*
	 set              (8BE2) : set 1536 char  & get   0
	 show             (8BF1) : set 1024 char  & get   0
	 get_site_survey  (8BED) : set   0       & get 1024 char
	 set_wsc_oob      (8BF9) : set 1024 char  & get 1024 char
	 get_mac_table    (8BEF) : set 1024 char  & get 1024 char
	 e2p              (8BE7) : set 1024 char  & get 1024 char
	 bbp              (8BE3) : set 1024 char  & get 1024 char
	 mac              (8BE5) : set 1024 char  & get 1024 char
	 rf               (8BF3) : set 1024 char  & get 1024 char
	 get_wsc_profile  (8BF2) : set 1024 char  & get 1024 char
	 get_ba_table     (8BF6) : set 1024 char  & get 1024 char
	 stat             (8BE9) : set 1024 char  & get 1024 char
	 elian            (8BFB) : set 512 char  & get 512 char
	 conn_status      (8BFC) : set 512 char  & get 512 char
	 */
	char buffer[4096];
	memset(buffer, 0, sizeof(buffer));
	iwpriv_conn_status("apcli0", buffer, sizeof(buffer));
	printf("iwpriv_conn_status [%s]\n", buffer);

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "AutoChannelSel=1");
	iwpriv_set("apcli0", buffer, sizeof(buffer));
	sleep(1);
	memset(buffer, 0, sizeof(buffer));
	iwpriv_get_site_survey("apcli0", buffer, sizeof(buffer));
	printf("iwpriv_get_site_survey [%s]\n", buffer);
	sleep(1);

	memset(buffer, 0, sizeof(buffer));
	strcpy(buffer, "result");
	iwpriv_elian("apcli0", buffer, sizeof(buffer));
	printf("iwpriv_elian [%s]\n", buffer);
}
/******************************************
 1,获取SSID与Pass
 2,扫描局域网WIFI,获取SSID的chl,与加密类型
 3,尝试连接WIFI

 ubus call network.interface.wan down
 ubus call network.interface.wan up
 wifi reload
 iwpriv apcli0 set ApCliEnable=0
 iwpriv apcli0 set ApCliSsid=cccccc
 iwpriv apcli0 set ApCliWPAPSK=hkdz1234567
 iwpriv apcli0 set ApCliAuthMode=WPA2PSK
 iwpriv apcli0 set ApCliEncrypType=AES
 iwpriv apcli0 set Channel=7
 iwpriv apcli0 set ApCliEnable=1
 iwpriv apcli0 show connStatus
 dmesg -c
 ifconfig apcli0
 iwconfig apcli0
 iwpriv apcli0 stat

 #扫描WIFI,这个时候会断开WIFI
 iwpriv apcli0 set AutoChannelSel=1
 iwpriv apcli0 set ACSCheckTime=1
 iwpriv apcli0 get_site_survey

 iwpriv apcli0 show stainfo

 iwpriv apcli0 set ApCliEnable=0
 iwpriv apcli0 set ApCliSsid=cccccc

 if [ "$ApCliWPAPSK"x = ""x ] ; then
 iwpriv apcli0 set ApCliAuthMode=OPEN
 iwpriv apcli0 set ApCliEncrypType=NONE
 else
 iwpriv apcli0 set ApCliWPAPSK=1234567
 iwpriv apcli0 set ApCliAuthMode=WPA2PSK
 iwpriv apcli0 set ApCliEncrypType=AES
 fi
 iwpriv apcli0 set Channel=7
 iwpriv apcli0 set ApCliEnable=1
 iwpriv apcli0 show connStatus
 ******************************************/
