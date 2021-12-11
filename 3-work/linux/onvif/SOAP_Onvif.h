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

#ifdef _MSC_VER	/* MSVC */
#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
 //#define vsnprintf _vsnprintf

#define GetSockError()	WSAGetLastError()
#define SetSockError(e)	WSASetLastError(e)
#define setsockopt(a,b,c,d,e)	(setsockopt)(a,b,c,(const char *)d,(int)e)

//#define EWOULDBLOCK	WSAETIMEDOUT	/* we don't use nonblocking, but we do use timeouts */

#define sleep(n)	Sleep(n*1000)
#define msleep(n)	Sleep(n)
#else
#define SOCKET  int 
#define closesocket(v) close(v)
#define GetSockError() errno
typedef unsigned int socklen_t;

#endif

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
	const char* myip,
		void (*bkfun)(void *context, const char *ipaddress, uint32_t ser_addr,
				struct OnvifDeviceProbeMatch *probe),
	void *context);

int Onvif_DiscoverIp(const char *ip, struct OnvifDeviceProbeMatch *probe);

struct OnvifDeviceInformation {
	int httpRespStatus;
	char Manufacturer[30];
	char Model[30];
	char FirmwareVersion[50];
	char SerialNumber[30];
	char HardwareId[20];
};

int Onvif_GetDeviceInformation(const char *url, const char *user,
		const char *pass, struct OnvifDeviceInformation *info);

int Onvif_GetCapabilities_Media(const char *url, const char *user,
		const char *pass, char *mediaUrl);

struct OnvifDeviceProfiles {
	int httpRespStatus;
	char token[100];
	char encodname[20];
	int width;
	int height;
	int frameRateLimit; //帧率
	int bitrateLimit;//码率
};

int Onvif_MediaService_GetProfiles(const char *mediaUrl,
	const char *user, 
	const char *pass,
	struct OnvifDeviceProfiles **profiles, 
	int* profsize);

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

void xml_label(const char *xmlstr,
			void (*callback)(const char *xmllabel, int llen, const char *txt,
						int tlen, void *user), void *user);

#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_LIBONVIF_SOAP_ONVIF_H_ */
