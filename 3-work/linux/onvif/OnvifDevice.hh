#ifndef _OnvifDevice_HH_
#define _OnvifDevice_HH_

extern "C" {
#include <string.h>
#include <stdio.h>

#include "SOAP_Onvif.h"

}

#include <string>
#include <vector>

namespace onvif {

struct URLInfo
{
	char prefix[10];
	char host[30];
	int port;
	char useranme[30];
	char password[30];
	std::string path;
	//const char* path;
};

int urldecode(const char* url, struct URLInfo* urlinfo);

class OnvifDevice {

public:
	char uri[500];
	char username[50];
	char password[50];
	bool ipreplaceflag;

	//设备信息
	struct OnvifDeviceInformation devInfo;

	//MediaURL
	char mediaUrl[300];

	//流
	struct OnvifDeviceProfiles* profiles;
	int profsize;

	std::vector<std::string> urls;
	//状态
	char stat[300];

	OnvifDevice(const char* uri, const char* username, const char* password, bool replaceipflag);
	~OnvifDevice();

	int GetDeviceInformation()
	{
		int ret = Onvif_GetDeviceInformation(uri, username, password, &devInfo);
		if (ret != 200)
			snprintf(stat, sizeof(stat), "GetDeviceInformation: %d", ret);
		return ret;
	}

	int GetCapabilities_Media() {		
		int ret = Onvif_GetCapabilities_Media(uri, username, password, mediaUrl);
		if (ret != 200)
			snprintf(stat,sizeof(stat), "GetCapabilities Media: %d", ret);
		else {
			if (ipreplaceflag) {
				struct URLInfo urlinfo_media;
				struct URLInfo urlinfo_uri;

				memset(&urlinfo_media, 0, sizeof(urlinfo_media));
				memset(&urlinfo_uri, 0, sizeof(urlinfo_uri));

				urldecode(mediaUrl, &urlinfo_media);
				urldecode(uri, &urlinfo_uri);

				if (strcmp(urlinfo_media.host, urlinfo_uri.host) != 0 || urlinfo_media.port != urlinfo_uri.port)
				{
					printf("不一样\r\n");
					char url[300];
					snprintf(url, sizeof(url), "%s%s:%d%s", urlinfo_uri.prefix, urlinfo_uri.host, urlinfo_uri.port, urlinfo_media.path.c_str());
					printf("新地址:%s\r\n", url);
					strncpy(mediaUrl, url, 300-1);
				}
				else {
					printf("media 一样\r\n");
				}
			}
		}
		return ret;
	}

	int MediaService_GetProfiles() {		
		profsize = 0;
		if (profiles)
			free(profiles);
		int ret = Onvif_MediaService_GetProfiles(mediaUrl, username, password, &profiles, &profsize);
		if (ret != 200) {		
			snprintf(stat, sizeof(stat), "MediaService GetProfiles: %d", ret);
		}
		return ret;
	}

	int MediaServer_GetStreamUri() {
		int i;
		int ret=0;
		char url[300];

		for (i = 0; i < profsize; i++)
		{
			if (strlen(profiles[i].token) == 0)
				break;
			//printf("token:%s, %dx%d, %s\r\n", profiles[i].token, profiles[i].width, profiles[i].height, profiles[i].encodname);
			memset(url, 0, 300);

			ret = Onvif_MediaServer_GetStreamUri(mediaUrl, username, password, profiles[i].token, url);
			OnvifURI_Decode(url, url, 300);
			urls.push_back(url);
		}
		if (ret != 200)
			snprintf(stat, sizeof(stat), "MediaServer GetStreamUri: %d", ret);
		return ret;
	}

	const char* getStatStr() {
		return stat;
	}
};

}
#endif