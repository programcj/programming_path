#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "wireless.h"
#include "iwtools.h"

static int iw_sockets_open(void) {
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

int iwpriv_show(const char *ifrname, const char *format, ...) {
	char buff[1024];
	va_list list;
	va_start(list, format);
	vsprintf(buff, format, list);
	va_end(list);
	return iwpriv_command(ifrname, 0x8BF1, buff, sizeof(buff));
}

int iwpriv_setarg(const char *ifrname, const char *cmds, ...) {
	char buff[1537];
	va_list list;
	va_start(list, cmds);
	vsprintf(buff, cmds, list);
	va_end(list);
	return iwpriv_set(ifrname, buff, sizeof(buff));
}

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

const struct wifi_info *iwpriv_getSiteSurvey(const char *netname, int *size) {
	static struct wifi_info wifi_list[50];
	char *buffer = NULL;
	char *tmp = NULL;
	char *line = NULL;
	struct wifi_info wifi;
	int i = 0;

	memset(&wifi_list, 0, sizeof(wifi_list));
	*size = i;

	buffer = (char*) malloc(10240);
	if (buffer == NULL)
		return wifi_list;

//	memset(buffer, 0, 10240); //iwpriv apcli0 set AutoChannelSel=1
//	strcpy(buffer, "SiteSurvey=");
//	if (iwpriv_set(netname, buffer, 10240) != 0) {
//		if (buffer)
//			free(buffer);
//		return wifi_list;
//	}
//	sleep(1);

	memset(buffer, 0, 10240);  //iwpriv apcli0 get_site_survey
	if (iwpriv_get_site_survey(netname, buffer, 10240) != 0) {
		if (buffer)
			free(buffer);
		return wifi_list;
	}
	//#define	LINE_LEN	(4+33+20+23+9+7+7+3)	/* Channel+SSID+Bssid+Security+Signal+WiressMode+ExtCh+NetworkType*/
	line = strtok_r(buffer, "\n", &tmp);
	while (line != NULL) {
		memset(&wifi, 0, sizeof(struct wifi_info));
		if (strlen(line) > (4 + 33 + 20 + 23 + 9 + 7) && strncasecmp(line, "Ch", 2) != 0) {
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
			if (i >= 50)
				break;
		}
		line = strtok_r(NULL, "\n", &tmp);
	}
	*size = i;
	if (buffer)
		free(buffer);
	return wifi_list;
}

/**
 * 获取接口连接的当前WIFI信息
 */
int iwconfig_get(const char *ifrname, char essid[33], uint8_t ap_add[6]) {
	struct iwreq wrq;
	int skfd = iw_sockets_open();
	if (skfd < 0)
		return -1;

	wrq.u.data.pointer = 0;
	wrq.u.data.length = 0;
	wrq.u.data.flags = 0;
	strncpy(wrq.ifr_name, ifrname, IFNAMSIZ);

	ioctl(skfd, SIOCGIWAP, &wrq);
	memcpy(ap_add, wrq.u.ap_addr.sa_data, 6);
	wrq.u.data.pointer = essid;
	wrq.u.data.length = 33;
	wrq.u.data.flags = 0;
	ioctl(skfd, SIOCGIWESSID, &wrq);
	/*
	 ioctl(skfd, SIOCGIWFREQ, &wrq);
	 {
	 //double res= ((double)wrq.u.freq.m) *pow(10, wrq.u.freq.e);
	 int i;
	 double res = (double) wrq.u.freq.m;
	 for (i = 0; i < wrq.u.freq.e; i++)
	 res *= 10;
	 printf("Channel:%f/%d\n", res, wrq.u.freq.flags);
	 }
	 */
	close(skfd);
	return 0;
}

int wifi_get(const char *ifrname, const char *ssid, struct wifi_info *wifi) {
	const struct wifi_info *wifis = NULL;
	int size = 0, i = 0;
	wifis = iwpriv_getSiteSurvey(ifrname, &size);
	for (i = 0; i < size; i++) {
		if (strcmp(wifis[i].ssid, ssid) == 0) {
			memcpy(wifi, &wifis[i], sizeof(struct wifi_info));
			return 0;
		}
	}
	return -1;
}

int wifi_connect_to(const char *ifrname, const char *ssid, const char *pass, struct wifi_info *wifi) {
	/*
	 连接WIFI部分
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
	 */
	iwpriv_setarg(ifrname, "ApCliEnable=0");
	iwpriv_setarg(ifrname, "ApCliSsid=%s", ssid);
	if (pass == NULL || strlen(pass) == 0) {
		iwpriv_setarg(ifrname, "ApCliAuthMode=OPEN");
		iwpriv_setarg(ifrname, "ApCliEncrypType=NONE");
	} else {
		iwpriv_setarg(ifrname, "ApCliWPAPSK=%s", pass);
		iwpriv_setarg(ifrname, "ApCliAuthMode=WPA2PSK");
		iwpriv_setarg(ifrname, "ApCliEncrypType=AES");
	}
	//_iwpriv_set(ifrname, "Channel=%d", wifi->ch);
	iwpriv_setarg(ifrname, "Channel=0");
	iwpriv_setarg(ifrname, "ApCliAutoConnect=1");
	return 0;
}

/*
 static char ssid[100];
 static char pass[100];
 static int opt_check = 0;

 void _arg_parse(int argc, char **argv) {
 int i = 0;

 while (i < argc) {
 if (strcmp(argv[i], "-v") == 0) {
 exit(1);
 }
 if (strcmp(argv[i], "-ssid") == 0) {
 i++;
 strcpy(ssid, argv[i]);
 continue;
 }
 if (strcmp(argv[i], "-pass") == 0) {
 i++;
 strcpy(pass, argv[i]);
 continue;
 }
 if (strcmp(argv[i], "-check") == 0) {
 i++;
 opt_check = 1;
 continue;
 }
 i++;
 }
 }

 int main(int argc, char **argv) {
 struct wifi_info wifi;
 int ret = 0;
 //char *ssid = "cccccc";
 //char *pass = "hkdz1234567";
 char *ifrname = "apcli0";
 char cur_ssid[100];
 uint8_t cur_ap[6];

 int cur_tim = 0;

 if (argc > 1) {
 //多参数:
 _arg_parse(argc, argv);
 }

 memset(&wifi, 0, sizeof(wifi));
 printf("scann ssid:%s\n", ssid);
 fflush(stdout);

 ret = wifi_get(ifrname, ssid, &wifi);
 if (ret) {
 printf("scan ssid err! not find.\n");
 }
 if (ret == 0) {
 printf("start connect ch:%d,ssid:%s, [%s]\n", wifi.ch, wifi.ssid, wifi.security);
 fflush(stdout);
 wifi_connect(ifrname, ssid, pass, &wifi);
 //监测WIFI连接是否成功
 cur_tim = time(NULL);
 while (time(NULL) - cur_tim < 5) {
 memset(cur_ssid, 0, sizeof(cur_ssid));
 iwconfig_get(ifrname,cur_ssid,cur_ap);
 if (strcmp(cur_ssid, ssid) == 0) {
 //wifi connect success
 printf("wifi connect success ssid:%s\n", cur_ssid);
 fflush(stdout);
 break;
 } else {
 memset(cur_ssid, 0, sizeof(cur_ssid));
 }
 sleep(1);
 }

 if (opt_check == 1) {  //5s监测WIFI是否能连接成功
 while (1) {
 memset(cur_ssid, 0, sizeof(cur_ssid));
 iwconfig_get(ifrname,cur_ssid,cur_ap);
 printf("wifi connect success, cur ssid:%s\n", cur_ssid);
 fflush(stdout);
 sleep(5);
 }
 }
 }

 return 0;
 }
 */
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
/*
 void debug_test() {
 char buffer[4096];
 struct iwreq wrq;
 char essid[33];

 memset(buffer, 0, sizeof(buffer));

 int skfd = iw_sockets_open();

 wrq.u.data.pointer = 0;
 wrq.u.data.length = 0;
 wrq.u.data.flags = 0;
 strncpy(wrq.ifr_name, "apcli0", IFNAMSIZ);
 ioctl(skfd, SIOCGIWAP, &wrq);

 uint8_t ap_add[6];
 memcpy(ap_add, wrq.u.ap_addr.sa_data, sizeof(ap_add));
 printf("Access Point:%02X:%02X:%02X:%02X:%02X:%02X\n", ap_add[0], ap_add[1], ap_add[2],
 ap_add[3], ap_add[4], ap_add[5]);

 wrq.u.data.pointer = essid;
 wrq.u.data.length = sizeof(essid);
 wrq.u.data.flags = 0;
 ioctl(skfd, SIOCGIWESSID, &wrq);

 printf("ESSID:%s\n", essid);

 ioctl(skfd, SIOCGIWFREQ, &wrq);

 {
 //double res= ((double)wrq.u.freq.m) *pow(10, wrq.u.freq.e);
 int i;
 double res = (double) wrq.u.freq.m;
 for (i = 0; i < wrq.u.freq.e; i++)
 res *= 10;

 printf("Channel:%f/%d\n", res, wrq.u.freq.flags);
 }

 close(skfd);

 }

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

 */
/******************************************
 1,获取SSID与Pass
 2,扫描局域网WIFI,获取SSID的chl,与加密类型
 3,尝试连接WIFI

 iwpriv apcli0 set ApCliEnable=0
 iwpriv apcli0 set ApCliSsid=cccccc
 iwpriv apcli0 set ApCliWPAPSK=hkdz1234567
 iwpriv apcli0 set ApCliAuthMode=WPA2PSK
 iwpriv apcli0 set ApCliEncrypType=AES
 iwpriv apcli0 set Channel=0
 iwpriv apcli0 set ApCliAutoConnect=1
 iwpriv apcli0 show connStatus

 ubus call network.interface.wan down
 ubus call network.interface.wan up
 wifi reload
 iwpriv apcli0 set ApCliEnable=0
 iwpriv apcli0 set ApCliSsid=cccccc
 iwpriv apcli0 set ApCliWPAPSK=hkdz1234567
 iwpriv apcli0 set ApCliAuthMode=WPA2PSK
 iwpriv apcli0 set ApCliEncrypType=AES
 iwpriv apcli0 set Channel=0
 iwpriv apcli0 set ApCliEnable=1
 iwpriv apcli0 set ApCliAutoConnect=1
 iwpriv apcli0 show connStatus

 dmesg -c
 ifconfig apcli0
 iwconfig apcli0
 iwpriv apcli0 stat

 iwpriv apcli0 set ACSCheckTime=1
 iwpriv apcli0 set AutoChannelSel=1
 #扫描WIFI,这个时候会断开WIFI
 iwpriv apcli0 set SiteSurvey=
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
/*
 int ifconfig_get_ip(const char *ifname, char *ipv4[16]) {
 struct ifreq ifr;
 int skfd;
 struct sockaddr_in *saddr;
 if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
 log_e("socket error");
 return -1;
 }
 strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
 if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
 //log_e("net_get_ifaddr: ioctl SIOCGIFADDR");
 close(skfd);
 return -1;
 }
 close(skfd);
 saddr = (struct sockaddr_in *) &ifr.ifr_addr;
 saddr->sin_addr.s_addr;

 //255.255.255.255
 inet_ntop(AF_INET, saddr->sin_addr.s_addr, ipv4, 16);
 return 0;
 //inet_ntoa(addr->sin_addr);
 }
 */
