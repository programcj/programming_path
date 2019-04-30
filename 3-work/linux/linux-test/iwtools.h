/*
 * iwtools.h
 *
 *  Created on: 2018年6月19日
 *      Author: cj
 */

#ifndef FILE_SRC_BIND_WIFI_IWTOOLS_H_
#define FILE_SRC_BIND_WIFI_IWTOOLS_H_

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

int iwpriv_command(const char *name, int iocmd, char *buff, int bufflen);

#define iwpriv_set(name, buff, bufflen) 	iwpriv_command(name, 0x8BE2, buff, bufflen)
#define iwpriv_get_site_survey(name, buff, bufflen) 	iwpriv_command(name, 0x8BED, buff, bufflen)
#define iwpriv_elian(name, buff, bufflen) 	iwpriv_command(name, 0x8BFB, buff, bufflen)

//#define iwpriv_conn_status(name, buff, bufflen) 	iwpriv_command(name, 0x8BFC, buff, bufflen)
int iwpriv_show(const char *ifrname, const char *format, ...);

int iwpriv_setarg(const char *ifrname, const char *cmds, ...) ;

const struct wifi_info *iwpriv_getSiteSurvey(const char *netname, int *size);

int iwconfig_get(const char *ifrname, char essid[33], uint8_t ap_add[6]);

#endif /* FILE_SRC_BIND_WIFI_IWTOOLS_H_ */
