/*
 * Onvif_Discover.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include "SOAP_Onvif.h"
#include "uuid_build.h"
#include "log_onvif.h"

#define XML_DISCOVER_Probe_tds_Device "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<Envelope xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:%s</wsa:MessageID>"\
	"<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"\
	"<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"\
	"</Header><Body><Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"\
	"<Types>tds:Device</Types><Scopes /></Probe>"\
	"</Body></Envelope>"

#define XML_DISCOVER_Probe_dn_NetworkVideoTransmitter \
			"<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
			"<Envelope xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" xmlns=\"http://www.w3.org/2003/05/soap-envelope\">"\
			"<Header><wsa:MessageID xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">uuid:%s</wsa:MessageID>"\
			"<wsa:To xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>"\
			"<wsa:Action xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>"\
			"</Header><Body>"\
			"<Probe xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"\
			"<Types>dn:NetworkVideoTransmitter</Types><Scopes /></Probe></Body></Envelope>"

int xml_probeMatch_parse(char *buff, int len,
			struct OnvifDeviceProbeMatch *probe)
{

	char *xml_XAddrs = NULL;
	char *xml_Address = NULL;
	char *xml_Scopes = NULL;
	char *xml_item_end = NULL;

	xml_Address = strstr(buff, "Address>");
	xml_Scopes = strstr(buff, "Scopes>");
	xml_XAddrs = strstr(buff, "XAddrs>"); //need get service address

	if (xml_Address)
	{
		xml_Address += strlen("Address>");
		xml_item_end = strstr(xml_Address, "</");
		if (xml_item_end)
			*xml_item_end = 0;
	}
	if (xml_Scopes)
	{
		xml_Scopes += strlen("Scopes>");
		xml_item_end = strstr(xml_Scopes, "</");
		if (xml_item_end)
			*xml_item_end = 0;
	}

	if (xml_XAddrs)
	{
		xml_XAddrs += strlen("XAddrs>");
		xml_item_end = strstr(xml_XAddrs, "</");
		if (xml_item_end)
			*xml_item_end = 0;
	}

	if (!xml_Address || !xml_Scopes || !xml_XAddrs)
		return -1;

	strncpy(probe->address, xml_Address, sizeof(probe->address));
	strncpy(probe->scopes, xml_Scopes, sizeof(probe->scopes));
	strncpy(probe->xaddrs, xml_XAddrs, sizeof(probe->xaddrs));
//	probe->address = strdup(xml_Address);
//	probe->scopes = strdup(xml_Scopes);
//	probe->xaddrs = strdup(xml_XAddrs);
	return 0;
}

uint32_t Tool_GetNetDevIPV4Str(const char *devName, char *ipstr)
{
	int sockfd = -1;
	struct ifreq ifr;
	struct sockaddr_in *addr = NULL;

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, devName);
	addr = (struct sockaddr_in *) &ifr.ifr_addr;
	addr->sin_family = AF_INET;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("create socket error!\n");
		return -1;
	}

	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0)
	{
		if (ipstr)
			inet_ntop(AF_INET, &addr->sin_addr, ipstr, 20);

		close(sockfd);
		return addr->sin_addr.s_addr;
	}
	close(sockfd);
	return -1;
}

static int socket_bind2(const char *devname, int port)
{
	struct sockaddr_in saddr;
	int sockfd;
	int ret;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = ntohs(port);
	//saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_addr.s_addr = Tool_GetNetDevIPV4Str(devname, NULL);

	int opt = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt,
				sizeof(opt)); //设置为可重复使用

	opt = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	//广播设置:能发送广播
	opt = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*) &opt,
				sizeof(opt));
	ret = bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr));
	if (ret < 0)
	{
		close(sockfd);
		sockfd = -1;
	}
	return sockfd;
}

void Onvif_DiscoverAll(int timeoutsec,
			void (*bkfun)(void *context, const char *ipaddress, uint32_t s_addr,
						struct OnvifDeviceProbeMatch *probe), void *context)
{
	struct sockaddr_in recvaddr;
	struct sockaddr_in broadcastaddr;
	char recvips[20];
	struct OnvifDeviceProbeMatch probe;

	int sockfd;
	int ret = 0;
	char uuid[37];
	char buff[5100];

	memset(&probe, 0, sizeof(struct OnvifDeviceProbeMatch));

	sockfd = socket_bind2("eth0", 3702);
	if (sockfd < 0)
	{
		fprintf(stderr, "port is exists bind 3702(Onvif)\n");
		return;
	}

	broadcastaddr.sin_family = AF_INET; //255.255.255.255 广播
	broadcastaddr.sin_port = htons(3702);
	broadcastaddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	uuid_build(uuid);
	snprintf(buff, sizeof(buff) - 1, XML_DISCOVER_Probe_tds_Device, uuid);
	sendto(sockfd, buff, strlen(buff), 0,
				(const struct sockaddr *) &broadcastaddr, sizeof(broadcastaddr));

	uuid_build(uuid);
	snprintf(buff, sizeof(buff) - 1,
	XML_DISCOVER_Probe_dn_NetworkVideoTransmitter, uuid);

	sendto(sockfd, buff, strlen(buff), 0,
				(const struct sockaddr *) &broadcastaddr, sizeof(broadcastaddr));

	{
		struct timeval timeout;
		timeout.tv_sec = timeoutsec; //秒
		timeout.tv_usec = 0; //微秒

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
					sizeof(timeout)) == -1)
		{
			fprintf(stderr, "setsockopt SO_RCVTIMEO failed! \n");
		}
	}

	struct pollfd fds;
	fds.fd = sockfd;
	fds.events = POLLIN; // 普通或优先级带数据可读

	while (1)
	{
		ret = poll(&fds, 1, 1000 * timeoutsec);
		if (-1 == ret)
		{
			perror("poll()");
			break;
		}
		if (ret == 0)
		{
			break;
		}
		if (POLLIN == (fds.revents & POLLIN))
		{
			socklen_t recvaddrlen = sizeof(recvaddr);
			memset(buff, 0, sizeof(buff));

			ret = recvfrom(sockfd, buff, sizeof(buff), 0,
						(struct sockaddr *) &recvaddr, &recvaddrlen);
			if (ret <= 0)
				continue;

			inet_ntop(AF_INET, &recvaddr.sin_addr.s_addr, recvips,
						sizeof(recvips));

			//need get service address
			ret = xml_probeMatch_parse(buff, ret, &probe);
			if (bkfun && !ret)
				bkfun(context, recvips, recvaddr.sin_addr.s_addr, &probe);
			//OnvifDeviceProbeMatch_clear(&probe);
		}
	}
	close(sockfd);
}

int socket_raw_udp(const char *devname)
{
	struct ifreq ifr;
	int fd;
	int ret;

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (fd < 0)
	{
		printf("socket_raw_udp:SOCK_RAW fd=%d, err=%d, %s\n", fd, errno,
					strerror(errno));
		return -1;
	}
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, devname);
	ret = ioctl(fd, SIOCGIFINDEX, &ifr);

	if (ret)
	{
		printf("SIOCGIFINDEX ret=%d, err=%d, %s\n", ret, errno,
					strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

#include <linux/ip.h>
#include <linux/udp.h>

static int udp_packet_parse(char *buff, int len)
{
	struct iphdr *pack_ip = (struct iphdr*) buff;
	struct udphdr *pack_udp = NULL;
	uint8_t *udp_data = NULL;
	int udp_data_len = 0;
	int iplen = (pack_ip->ihl & 0x0f) * 4;
	char sip[20];
	char dip[20];

	if (pack_ip->protocol != IPPROTO_UDP)
		return -1;
	inet_ntop(AF_INET, &pack_ip->saddr, sip, sizeof(sip));
	inet_ntop(AF_INET, &pack_ip->daddr, dip, sizeof(dip));

	pack_udp = (struct udphdr*) (buff + iplen);
	udp_data = buff + (iplen + sizeof(struct udphdr));
	udp_data_len = ntohs(pack_udp->len) - sizeof(struct udphdr);

	uint16_t sport = ntohs(pack_udp->source);
	uint16_t dport = ntohs(pack_udp->dest);

	if (sport == 3702)
		printf("udp %s:%d->%s:%d, len:%d, %s\n", sip, sport, dip, dport,
					udp_data_len, udp_data);
	return 0;
}

/********
 * %20 是空格哦
 <?xml version="1.0" encoding="utf-8"?>
 <env:Envelope xmlns:env="http://www.w3.org/2003/05/soap-envelope" xmlns:soapenc="http://www.w3.org/2003/05/soap-encoding" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:tt="http://www.onvif.org/ver10/schema" xmlns:tds="http://www.onvif.org/ver10/device/wsdl" xmlns:trt="http://www.onvif.org/ver10/media/wsdl" xmlns:timg="http://www.onvif.org/ver20/imaging/wsdl" xmlns:tev="http://www.onvif.org/ver10/events/wsdl" xmlns:tptz="http://www.onvif.org/ver20/ptz/wsdl" xmlns:tan="http://www.onvif.org/ver20/analytics/wsdl" xmlns:tst="http://www.onvif.org/ver10/storage/wsdl" xmlns:ter="http://www.onvif.org/ver10/error" xmlns:dn="http://www.onvif.org/ver10/network/wsdl" xmlns:tns1="http://www.onvif.org/ver10/topics" xmlns:tmd="http://www.onvif.org/ver10/deviceIO/wsdl" xmlns:wsdl="http://schemas.xmlsoap.org/wsdl" xmlns:wsoap12="http://schemas.xmlsoap.org/wsdl/soap12" xmlns:http="http://schemas.xmlsoap.org/wsdl/http" xmlns:d="http://schemas.xmlsoap.org/ws/2005/04/discovery" xmlns:wsadis="http://schemas.xmlsoap.org/ws/2004/08/addressing" xmlns:wsnt="http://docs.oasis-open.org/wsn/b-2" xmlns:wsa="http://www.w3.org/2005/08/addressing" xmlns:wstop="http://docs.oasis-open.org/wsn/t-1" xmlns:wsrf-bf="http://docs.oasis-open.org/wsrf/bf-2" xmlns:wsntw="http://docs.oasis-open.org/wsn/bw-2" xmlns:wsrf-rw="http://docs.oasis-open.org/wsrf/rw-2" xmlns:wsaw="http://www.w3.org/2006/05/addressing/wsdl" xmlns:wsrf-r="http://docs.oasis-open.org/wsrf/r-2" xmlns:trc="http://www.onvif.org/ver10/recording/wsdl" xmlns:tse="http://www.onvif.org/ver10/search/wsdl" xmlns:trp="http://www.onvif.org/ver10/replay/wsdl" xmlns:tnshik="http://www.hikvision.com/2011/event/topics" xmlns:hikwsd="http://www.onvifext.com/onvif/ext/ver10/wsdl" xmlns:hikxsd="http://www.onvifext.com/onvif/ext/ver10/schema" xmlns:tas="http://www.onvif.org/ver10/advancedsecurity/wsdl">
 <env:Header>
 <wsadis:MessageID>urn:uuid:18ec6401-3aa0-11b5-83f6-b4a3820fd92d</wsadis:MessageID>
 <wsadis:RelatesTo>uuid:bfe8bb88-03a2-4e1a-904a-59a7d2882649</wsadis:RelatesTo>
 <wsadis:To>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</wsadis:To>
 <wsadis:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</wsadis:Action>
 <d:AppSequence InstanceId="705289" MessageNumber="11682"/>
 </env:Header>
 <env:Body>
 <d:ProbeMatches>
 <d:ProbeMatch>
 <wsadis:EndpointReference>
 <wsadis:Address>urn:uuid:18ec6401-3aa0-11b5-83f6-b4a3820fd92d</wsadis:Address>
 </wsadis:EndpointReference>
 <d:Types>dn:NetworkVideoTransmitter tds:Device</d:Types>
 <d:Scopes>
 onvif://www.onvif.org/type/video_encoder onvif://www.onvif.org/Profile/Streaming
 onvif://www.onvif.org/Profile/G onvif://www.onvif.org/type/audio_encoder
 onvif://www.onvif.org/type/ptz onvif://www.onvif.org/hardware/DS-2CD3955FWD-IWS
 onvif://www.onvif.org/name/HIKVISION%20DS-2CD3955FWD-IWS onvif://www.onvif.org/location/city/hangzhou
 </d:Scopes>
 <d:XAddrs>http://192.168.0.218/onvif/device_service http://[fe80::b6a3:82ff:fe0f:d92d]/onvif/device_service</d:XAddrs>
 <d:MetadataVersion>10</d:MetadataVersion>
 </d:ProbeMatch>
 </d:ProbeMatches>
 </env:Body>
 </env:Envelope>
 ***********/
int Onvif_DiscoverIp(const char *ip, struct OnvifDeviceProbeMatch *probe)
{
	int sockfd;
	//int udpfd;
	char buff[10240];
	char uuid[37];
	struct sockaddr_in dscAddr;
	int ret;
	int exists = 0;

	memset(probe, 0, sizeof(struct OnvifDeviceProbeMatch));

	sockfd = socket_bind2("eth0", 0);
	//udpfd = socket_raw_udp("eth0");
	memset(&dscAddr, 0, sizeof(dscAddr));

	dscAddr.sin_family = AF_INET; //255.255.255.255 广播
	dscAddr.sin_port = htons(3702);
	dscAddr.sin_addr.s_addr = inet_addr(ip); //htonl(INADDR_BROADCAST);

	uuid_build(uuid);
	snprintf(buff, sizeof(buff) - 1, XML_DISCOVER_Probe_tds_Device, uuid);
	sendto(sockfd, buff, strlen(buff), 0, (const struct sockaddr *) &dscAddr,
				sizeof(dscAddr));

	uuid_build(uuid);
	snprintf(buff, sizeof(buff) - 1,
	XML_DISCOVER_Probe_dn_NetworkVideoTransmitter, uuid);

	sendto(sockfd, buff, strlen(buff), 0, (const struct sockaddr *) &dscAddr,
				sizeof(dscAddr));

	{
		struct timeval timeout;
		timeout.tv_sec = 2; //秒
		timeout.tv_usec = 0; //微秒

		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
					sizeof(timeout)) == -1)
		{
			fprintf(stderr, "setsockopt SO_RCVTIMEO failed! \n");
		}
	}

	char recvips[20];
	while (1)
	{
		memset(buff, 0, sizeof(buff));

		if (sockfd < 0)
			break;

		socklen_t recvaddrlen = sizeof(dscAddr);
		ret = recvfrom(sockfd, buff, sizeof(buff), 0,
					(struct sockaddr *) &dscAddr, &recvaddrlen);
		if (ret <= 0)
			break;
		{
			char *xml_item_end = NULL;

			memset(recvips, 0, sizeof(recvips));
			inet_ntop(AF_INET, &dscAddr.sin_addr.s_addr, recvips,
						sizeof(recvips));
			if (0 != strcasecmp(recvips, ip))
				continue;

			if (!xml_probeMatch_parse(buff, ret, probe))
			{
				exists = 1;
				break;
			}
		}
#if 0
		rlen = recv(udpfd, buff, sizeof(buff), 0);
		if (rlen > 0)
		{
			udp_packet_parse(buff, rlen);
		}
#endif
	}
	if (exists)
	{
//		char *buf = buff;
//		char *tmp = NULL;
//		char *p = NULL;
//		printf("IP:%s, xml_XAddrs0:\n", recvips);
//		while ((p = strtok_r(buf, " ", &tmp)) != NULL) {
//			printf("%s\n", p);
//
//			char *url=strdup(p);
//
//			buf=NULL;
//		}
	}
	if (sockfd > 0)
		close(sockfd);
	if (exists)
		return 0;
	return -1;
}

struct OldAll
{
	void *context;
	void (*bkfun)(void *context, uint32_t addr, const char *XAddrs);
};

void _oldAll(void *context, const char *ipaddress, uint32_t s_addr,
			struct OnvifDeviceProbeMatch *probe)
{
	struct OldAll *old = (struct OldAll*) context;
	old->bkfun(old->context, s_addr, probe->xaddrs);
}

void OnvifDiscoverAll(int timeoutsec,
			void (*bkfun)(void *context, uint32_t addr, const char *XAddrs),
			void *context)
{
	struct OldAll old;
	old.context = context;
	old.bkfun = bkfun;
	Onvif_DiscoverAll(timeoutsec, _oldAll, &old);
}

#include "Onvif.h"

struct OnvifDeviceInfo
{
	struct OnvifDeviceProbeMatch probe;
	struct OnvifDeviceInformation devInfo;
	char mediaUrl[300];
	struct OnvifDeviceProfiles profiles[5];
	char uri[5][300];
};

int OnvifGetIpcDataByURL(const char *url, const char *usename, const char *password,
			ONVIF_IPC_DATA_S *ipcData)
{
	struct OnvifDeviceInfo _onvifinfo;
	struct OnvifDeviceInfo *onvifInfo = &_onvifinfo;
	char *serverUrl = NULL;
	int ret;

	memset(onvifInfo, 0, sizeof(_onvifinfo));
	memset(ipcData, 0, sizeof(ONVIF_IPC_DATA_S));

	strcpy(onvifInfo->mediaUrl, url);
	if (onvifInfo->profiles[0].httpRespStatus == 0)
	{
		memset(onvifInfo->profiles, 0, sizeof(onvifInfo->profiles));
		ret = Onvif_MediaService_GetProfiles(onvifInfo->mediaUrl, usename,
					password, onvifInfo->profiles);
		if (ret == 401)
		{
			log_e("onvif",
						"Onvif_MediaService_GetProfiles %s,[%s][%s] user or passowrd err\n",
						url, usename, password);
			return ret;
		}
		if (ret != 200)
		{
			log_e("onvif", "Onvif_MediaService_GetProfiles err [ret=%d] %s\n", ret, onvifInfo->mediaUrl);
			onvifInfo->profiles[0].httpRespStatus = 0;
			return ret;
		}
	}

	int i = 0;

	for (i = 0; i < 5; i++)
	{
		if (strlen(onvifInfo->profiles[i].token) == 0)
			break;
		memset(onvifInfo->uri[i], 0, 300);

		ret = Onvif_MediaServer_GetStreamUri(onvifInfo->mediaUrl, usename,
					password, onvifInfo->profiles[i].token, onvifInfo->uri[i]);

		if (ret == 200)
		{
			OnvifURI_Decode(onvifInfo->uri[i], onvifInfo->uri[i], 300);
		}
		log_i("onvif", "token:%s, uri:%s\n", onvifInfo->profiles[i].token,
					onvifInfo->uri[i]);
		//break;
	}

	strncpy(ipcData->url.rtspUrl, onvifInfo->uri[0], 256 - 1);
	strncpy(ipcData->url.rtspUrlSub, onvifInfo->uri[1], 256 - 1);
	strcpy(ipcData->devData.brand, onvifInfo->devInfo.Manufacturer);
	strcpy(ipcData->devData.firmwareversion,
				onvifInfo->devInfo.FirmwareVersion);
	strcpy(ipcData->devData.model, onvifInfo->devInfo.Model);
	strcpy(ipcData->devData.serialnumber, onvifInfo->devInfo.SerialNumber);

	log_i("onvif", "IPC:[%s][%s]\n", ipcData->url.rtspUrl, ipcData->url.rtspUrlSub);
	return ret;
}

int OnvifGetIpcDataByIp(const char *iporurl, const char *usename, const char *password,
			ONVIF_IPC_DATA_S *ipcData)
{
	struct OnvifDeviceInfo _onvifinfo;
	struct OnvifDeviceInfo *onvifInfo = &_onvifinfo;
	char *serverUrl = NULL;
	int ret;
	memset(onvifInfo, 0, sizeof(_onvifinfo));
	memset(ipcData, 0, sizeof(ONVIF_IPC_DATA_S));
	const char *ip = iporurl;
	//
	if (strncasecmp(iporurl, "http://", strlen("http://")) == 0)
	{
		snprintf(onvifInfo->probe.xaddrs, 500, "%s/onvif/device_service", iporurl);
	}
	else
	if (strlen(onvifInfo->probe.xaddrs) == 0)
	{
		ret = Onvif_DiscoverIp(ip, &onvifInfo->probe);
		if (ret < 0)
		{
			log_w("onvif", "设备不存在:%d, %s,%s,%s,尝试默认地址\n", ret, ip, usename, password);
			snprintf(onvifInfo->probe.xaddrs, 500, "http://%s:80/onvif/device_service", ip);
			//return -2;
		}

		log_d("onvif", "xaddr:%s\n", onvifInfo->probe.xaddrs);
		{
			char *url = strchr(onvifInfo->probe.xaddrs, ' ');
			if (url)
				*url = 0;
			//strtok_r(buf, " ", &tmp)
			//strcpy(probe.xaddrs, "http://192.168.0.122");
		}
	}
	serverUrl = onvifInfo->probe.xaddrs;

	log_d("onvif", "OnvifDevice:%s,%s,%s[%s]\n", ip, usename, password,
				serverUrl);
	//exit(1);
	if (onvifInfo->devInfo.httpRespStatus == 0)
	{
		ret = Onvif_GetDeviceInformation(serverUrl, usename, password,
					&onvifInfo->devInfo);
		if (ret == 401)
		{
			log_w("onvif",
						"onvif GetDeviceInformation %s,[%s][%s] user or passowrd err\n",
						ip, usename, password);
			//return ret;
		}
		if (ret != 200)
		{
			log_w("onvif", "onvif GetDeviceInformation err,ret=%d, %s,%s,%s\n", ret,
						ip, usename, password);
			onvifInfo->devInfo.httpRespStatus = 0;			//下次获取
			//return ret;
		}
		if (ret == 200)
			log_d("onvif", "Onvif info, %s, mode:%s,mau:%s,fmv:%s, sid:%s\n", ip,
						onvifInfo->devInfo.Model, onvifInfo->devInfo.Manufacturer,
						onvifInfo->devInfo.FirmwareVersion,
						onvifInfo->devInfo.SerialNumber);
	}

	ret = Onvif_GetCapabilities_Media(serverUrl, usename, password,
				onvifInfo->mediaUrl);

	if (ret != 200)
	{
		log_e("onvif", "Onvif_GetCapabilities_Media %s, not get media url\n", ip);
		return ret;
	}

	log_d("onvif", "media url:%s\n", onvifInfo->mediaUrl);

//	if (strcmp(onvifInfo->mediaUrl, serverUrl) != 0)
//	{
//		strcpy(onvifInfo->mediaUrl, "http://e4105f8dd3a28f182b5440ab500713e6.192.168.1.241.80.oms.lxvision.com:8100/onvif/service");
//	}

	if (onvifInfo->profiles[0].httpRespStatus == 0)
	{
		memset(onvifInfo->profiles, 0, sizeof(onvifInfo->profiles));
		ret = Onvif_MediaService_GetProfiles(onvifInfo->mediaUrl, usename,
					password, onvifInfo->profiles);
		if (ret == 401)
		{
			log_e("onvif",
						"Onvif_MediaService_GetProfiles %s,[%s][%s] user or passowrd err\n",
						ip, usename, password);
			return ret;
		}
		if (ret != 200)
		{
			log_e("onvif", "Onvif_MediaService_GetProfiles err [ret=%d] %s\n", ret, onvifInfo->mediaUrl);
			onvifInfo->profiles[0].httpRespStatus = 0;
			return ret;
		}
	}

	int i = 0;

	for (i = 0; i < 5; i++)
	{
		if (strlen(onvifInfo->profiles[i].token) == 0)
			break;
		memset(onvifInfo->uri[i], 0, 300);

		ret = Onvif_MediaServer_GetStreamUri(onvifInfo->mediaUrl, usename,
					password, onvifInfo->profiles[i].token, onvifInfo->uri[i]);

		if (ret == 200)
		{
			OnvifURI_Decode(onvifInfo->uri[i], onvifInfo->uri[i], 300);
		}
		log_i("onvif", "token:%s, uri:%s\n", onvifInfo->profiles[i].token,
					onvifInfo->uri[i]);
		//break;
	}

	strncpy(ipcData->url.rtspUrl, onvifInfo->uri[0], 256 - 1);
	strncpy(ipcData->url.rtspUrlSub, onvifInfo->uri[1], 256 - 1);
	strcpy(ipcData->devData.brand, onvifInfo->devInfo.Manufacturer);
	strcpy(ipcData->devData.firmwareversion,
				onvifInfo->devInfo.FirmwareVersion);
	strcpy(ipcData->devData.model, onvifInfo->devInfo.Model);
	strcpy(ipcData->devData.serialnumber, onvifInfo->devInfo.SerialNumber);

	log_i("onvif", "IPC:[%s][%s]\n", ipcData->url.rtspUrl, ipcData->url.rtspUrlSub);
	return ret;
}

