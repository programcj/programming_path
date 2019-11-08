/*
 * Onvif.h
 *
 *  Created on: 2019年11月7日
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

#ifndef SRC_INC_LIBONVIF_SOAP_ONVIF_H_
#define SRC_INC_LIBONVIF_SOAP_ONVIF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct OnvifDeviceProbeMatch {
	char address[50];
	//char *types;
	char scopes[500];
	char xaddrs[500];
};

void Onvif_DiscoverAll(int timeoutsec,
		void (*bkfun)(void *context, const char *ipaddress, uint32_t s_addr,
				struct OnvifDeviceProbeMatch *probe), void *context);

void OnvifDeviceProbeMatch_clear(struct OnvifDeviceProbeMatch *p);

int Onvif_DiscoverIp(const char *ip, struct OnvifDeviceProbeMatch *probe);

struct OnvifDeviceInformation {
	char Manufacturer[30];
	char Model[30];
	char FirmwareVersion[50];
	char SerialNumber[30];
	char HardwareId[20];
};

int Onvif_GetDeviceInformation(const char *url, const char *user,
		const char *pass, struct OnvifDeviceInformation *info);

int Onvif_GetCapabilities_Media(url, user, pass, mediaUrl);

struct OnvifDeviceProfiles {
	char token[100];
	int width;
	int height;
};

int Onvif_MediaService_GetProfiles(const char *mediaUrl, const char *user,
		const char *pass, struct OnvifDeviceProfiles profiles[5]);

int Onvif_MediaServer_GetStreamUri(const char *url, const char *user,
		const char *pass, const char *ProfileToken, char uri[300]);

void OnvifURI_Decode(const char *url, char *desc, int dlen);

/**
 Onvif_GetDeviceInformation(url, user, pass, &info);
 Onvif_GetCapabilities_Media(url, user, pass, mediaUrl);
 Onvif_MediaService_GetProfiles(mediaUrl, user, pass, profiles);
 Onvif_MediaServer_GetStreamUri(mediaUrl, user, pass, profiles[i].token, uri);
 OnvifURI_Decode(uri, uri, 300);

 */
#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_LIBONVIF_SOAP_ONVIF_H_ */
