/*
 * Onvif_HttpDigest.c
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "curl/curl.h"
#include "digest.h"
#include "client.h"
#include "SOAP_Onvif.h"

static int http_content_point(const char* txt) {
	const char* ptr = txt;
	while (ptr && *ptr) {
		if (*ptr == '\n') {
			if (*(ptr + 1) == '\r' || *(ptr + 1) == '\n')
				break;
		}
		ptr++;
	}
	if (ptr && *ptr) {
		while (*ptr) {
			if (*ptr != '\r' && *ptr != '\n') {
				break;
			}
			ptr++;
		}
	}
	return ptr - txt;
}

static void _cnonce_buildstr(char randstr[33]) {
	int i = 0;
	unsigned short v = 0;
	srand((unsigned) time(NULL)); /*播种子*/
	for (i = 0; i < 32; i += 2) {
		v = rand() % 100;/*产生随机整数*/
		sprintf(randstr + i, "%02X", v);
	}
	randstr[32] = 0;
}

static int http_auth_digest(const char* dsc_disget, char* resultstr, int reslen,
		const char* username, const char* password, const char *uri) {
	//https://github.com/jacketizer/libdigest
	digest_t d;
	char cnonce[33];

	if (-1 == digest_is_digest(dsc_disget)) {
		fprintf(stderr, "Could not digest_is_digest!\n");
		return -1;
	}

	digest_init(&d);

	digest_client_parse(&d, dsc_disget);

	digest_set_attr(&d, D_ATTR_USERNAME, (digest_attr_value_t) username);
	digest_set_attr(&d, D_ATTR_PASSWORD, (digest_attr_value_t) password);
	digest_set_attr(&d, D_ATTR_URI, (digest_attr_value_t) uri);

	//char *cnonce = "400616322553302B623F0A0C514B0543";
	_cnonce_buildstr(cnonce);

	digest_set_attr(&d, D_ATTR_CNONCE, (digest_attr_value_t) cnonce);

	digest_set_attr(&d, D_ATTR_ALGORITHM, (digest_attr_value_t) 1);
	digest_set_attr(&d, D_ATTR_METHOD,
			(digest_attr_value_t) DIGEST_METHOD_POST);

	if (-1 == digest_client_generate_header(&d, resultstr, reslen)) {
		fprintf(stderr, "Could not build the Authorization header!\n");
	}
	return 0;
}

struct data_buff {
	char *buff;
	int size;
	int index;
};

static size_t _http_curl_wheader(char* buffer, size_t size, //大小
		size_t nitems, //哪一块
		struct data_buff *out) {
	//strncat(out, buffer, size * nitems);
	memcpy(out->buff + out->index, buffer, size * nitems);
	out->index += size * nitems;
	return size * nitems;
}

static size_t _http_curl_wcontent(char* buffer, size_t size, //大小
		size_t nitems, //哪一块
		struct data_buff *out) {
	int len = size * nitems;
	if (len + out->index < out->size) {
		memcpy(out->buff + out->index, buffer, len);
		out->index += len;
	}
	return size * nitems;
}

static int http_post(const char *url, const char *authorization,
		const char *poststr, char *respstr, int resplen) {
	CURL* curl;
	CURLcode res;
	struct curl_slist* http_header = NULL;
	struct data_buff buff;

	buff.buff = respstr;
	buff.size = resplen;
	buff.index = 0;

	http_header = curl_slist_append(http_header,
			"Content-Type: application/soap+xml; charset=utf-8");
	if (authorization)
		http_header = curl_slist_append(http_header, authorization);

	// 初始化CURL
	curl = curl_easy_init();
	if (!curl) {
		perror("curl init failed \n");
		return -1;
	}

	// 设置CURL参数
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

	curl_easy_setopt(curl, CURLOPT_URL, url); //url地址
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, poststr); //post参数
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(poststr));

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_curl_wheader); //处理http头部
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &buff); //http头回调数据

	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  //将响应头信息和相应体一起传给write_data

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_curl_wcontent); //处理http内容
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buff); //这是write_data的第四个参数值

	curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, resplen - 1);
	curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息

	//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
	//curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/tmp/curlpost.cookie");

	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L); //接收数据时超时设置，如果10秒内数据未接收完，直接退出
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0); //设为false 下面才能设置进度响应函数

	res = curl_easy_perform(curl);

	long httpcode = 0;
	// 判断是否接收成功
	if (res != CURLE_OK) { //CURLE_OK is 0
		fprintf(stderr, "-->CURL err: %s,%d \n", url, res);

		long headersize = 0;
		res = curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &headersize);
		res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcode);
		res = httpcode;
		printf("[headersize:%ld]res=%d\n", headersize, res);
	}

	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpcode);
	res = httpcode;

	if (httpcode == 200) {
		int sumlen = strlen(respstr);
		int cpoit = http_content_point(respstr);
		if (cpoit < resplen)
			memcpy(respstr, respstr + cpoit, resplen - cpoit);
	}

	// 释放CURL相关分配内存
	curl_slist_free_all(http_header);
	curl_easy_cleanup(curl);
	return res;
}

/**
 * onvif的http请求, return 200成功
 */
int onvif_http_digest_post(const char *url, const char *username,
		const char *password, const char *postStr, char *contentText,
		int contentTextlen) {
	char *authenticate401 = NULL, *ptr = NULL;
	char authorization[300];
	char uri[100];
	int ret = 0;

	memset(contentText, 0, contentTextlen);
	ret = http_post(url, NULL, postStr, contentText, contentTextlen);
	if (ret == 200) {
		return 200;
	}

	if (ret != 401) {
		return ret;
	}

	if (NULL == username || NULL == password) {
		return -1;
	}

	authenticate401 = strstr(contentText, "WWW-Authenticate:");
	if (!authenticate401)
		return -1;

	authenticate401 = strchr(authenticate401, ':');
	if (!authenticate401)
		return -1;

	authenticate401++;

	while (authenticate401 && *authenticate401 == ' '
			&& *authenticate401 != '\n' && *authenticate401 != '\r'
			&& *authenticate401 != 0)
		authenticate401++;

	ptr = authenticate401;
	while (ptr && *ptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			*ptr = 0;
			break;
		}
		ptr++;
	}

	memset(authorization, 0, sizeof(authorization));

	int namelen = strlen("Authorization: ");
	sprintf(authorization, "Authorization: ");

	//printf("authenticate401=%s\n", authenticate401);

	{
		char *next = strstr(url, "://");
		memset(uri, 0, sizeof(uri));
		if (next) {
			next += 3;
			next = strchr(next, '/');
			if (next) {
				strncpy(uri, next, sizeof(uri) - 1);
			}
		}
	}

	//Authorization: Digest username="admin", realm="Login to 2J01018PAA00813", qop="auth", algorithm="MD5", uri="/onvif/device_service", nonce="b252aWYtZGlnZXN0OjQzNTIyMDY3OTEw", nc=00000001, cnonce="AD484FC8EC7964EDA1EC6F89FC8A93CC", opaque="", response="6433f6224f6efe9a6102afcdc707b5ec"
	ret = http_auth_digest(authenticate401, authorization + namelen,
			sizeof(authorization) - namelen, username, password, uri);

	contentText[0] = 0;

	ret = http_post(url, authorization, postStr, contentText, contentTextlen);
	return ret;
}

#define XML_GetDeviceInformation \
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
		"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
		"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
		"<GetDeviceInformation xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>"\
		"</s:Body></s:Envelope>"

//<tds:GetDeviceInformationResponse>
//  <tds:Manufacturer>Dahua</tds:Manufacturer>
//  <tds:Model>IPC-HDW1025C</tds:Model>
//  <tds:FirmwareVersion>2.420.Dahua 00.14.R, build: 2016-06-18</tds:FirmwareVersion>
//  <tds:SerialNumber>2J01018PAA00813</tds:SerialNumber>
//  <tds:HardwareId>1.00</tds:HardwareId>
//</tds:GetDeviceInformationResponse>

struct xml_node {
	char *name;
	char *text;
};

static struct xml_node *xml_node_alloc() {
	return (struct xml_node *) calloc(sizeof(struct xml_node), 1);
}

static void xml_node_Delete(struct xml_node *node) {
	if (node) {
		if (node->name)
			free(node->name);
		if (node->text)
			free(node->text);
		free(node);
	}
}

static char *strchr_end(const char *str_head, char *str_end, int c) {
	while (str_end >= str_head) {
		if (*str_end == c)
			return str_end;
		str_end--;
	}
	return NULL;
}

static struct xml_node *xml_getNode(const char *xmlstr, const char *name) {
	char *ps = NULL;
	char *ps_end = NULL;
	struct xml_node *node = NULL;

	ps = strstr(xmlstr, name);
	if (ps && *ps != '<') {
		ps = strchr_end(xmlstr, ps, '<');
		if (ps)
			ps++;
	}
	if (!ps)
		return NULL;

	ps_end = strchr(ps, '>');
	if (ps_end == NULL)
		return NULL;

	int len = ps_end - ps;
	ps_end = strstr(ps, "</");
	if (!ps_end)
		return NULL;
	node = xml_node_alloc();
	node->name = (char*) malloc(len);
	strncpy(node->name, ps, len - 1);

	ps += len + 1;
	len = ps_end - ps + 1;
	node->text = (char*) malloc(len);
	node->text[len - 1] = 0;
	strncpy(node->text, ps, len - 1);
	return node;
}

int Onvif_GetDeviceInformation(const char *url, const char *user,
		const char *pass, struct OnvifDeviceInformation *info) {
	char xmlstr[10240];
	int ret = 0;
	struct xml_node *node;

	xmlstr[0] = 0;
	ret = onvif_http_digest_post(url, user, pass, XML_GetDeviceInformation,
			xmlstr, sizeof(xmlstr));

	node = xml_getNode(xmlstr, "Manufacturer>");
	if (node) {
		strncpy(info->Manufacturer, node->text, sizeof(info->Manufacturer));
	}
	xml_node_Delete(node);
	node = xml_getNode(xmlstr, "Model>");
	if (node) {
		strncpy(info->Model, node->text, sizeof(info->Model));
	}
	xml_node_Delete(node);
	node = xml_getNode(xmlstr, "FirmwareVersion>");
	if (node) {
		strncpy(info->FirmwareVersion, node->text,
				sizeof(info->FirmwareVersion));
	}
	xml_node_Delete(node);
	node = xml_getNode(xmlstr, "SerialNumber>");
	if (node) {
		strncpy(info->SerialNumber, node->text, sizeof(info->SerialNumber));
	}
	xml_node_Delete(node);
	node = xml_getNode(xmlstr, "HardwareId>");
	if (node) {
		strncpy(info->HardwareId, node->text, sizeof(info->HardwareId));
	}
	xml_node_Delete(node);
	return ret;
}
#define XML_GetCapabilities_Media "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\">"\
	"<Category>Media</Category>"\
	"</GetCapabilities>"\
	"</s:Body></s:Envelope>"

int Onvif_GetCapabilities_Media(const char *url, const char *user,
		const char *pass, char *mediaUrl) {
	char xmlstr[10240];
	int ret = 0;
	struct xml_node *node;

	xmlstr[0] = 0;
	ret = onvif_http_digest_post(url, user, pass, XML_GetCapabilities_Media,
			xmlstr, sizeof(xmlstr));
	char *xmlMedia = strstr(xmlstr, "Media>");
	if (xmlMedia) {
		node = xml_getNode(xmlMedia, "XAddr>");
		if (node) {
			strcpy(mediaUrl, node->text);
		}
		xml_node_Delete(node);
	}
	return ret;
}

int printstr(const char *p, int len) {
	const char *end = p + len;
	int ret = 0;
	while (p < end) {
		putchar(*p++);
		ret++;
	}
	return ret;
}

static char *xml_findend(const char *xmlstr, const char *labelname,
		int labellen) {
	const char *p = xmlstr;
	const char *end = xmlstr + strlen(xmlstr);
	while (p < end) {
		if (*p == '<' && *(p + 1) == '/') {
			p += 2;
			if (strncmp(p, labelname, labellen) == 0) {
				return (char*) p - 2;
			}
		}
		p++;
	}
	return NULL;
}

void xml_label_out(const char *xmllabel, int llen, const char *txt, int tlen,
		void *user) {
	printf("label:");
	printstr(xmllabel, llen);
	printf("\n");

	printf(" txt:");
	if (txt[0] != '<')
		printstr(txt, tlen);
	else
		printf("[xml format]");
	printf("\n");
}

void xml_label(const char *xmlstr,
		void (*callback)(const char *xmllabel, int llen, const char *txt,
				int tlen, void *user), void *user) {
	const char *p = xmlstr;
	const char *end = xmlstr + strlen(xmlstr);
	char *lend = NULL;
	const char *txt_s = NULL;
	const char *txt_e = NULL;
	const char *lname_end;
	const char *lname_s;

	while (p < end) {
		//xml 开始

		if (*p == '<' && *(p + 1) != '/' && p + 1 < end) {
			p++;

			lend = strchr(p, '>');
			if (lend) {
				if (*(lend - 1) == '/') {
					lend--;
				}
				lname_end = strchr(p, ' ');
				if (lname_end > lend || !lname_end)
					lname_end = lend;
				lname_s = p;

				if (*lend == '/') {
					txt_s = "";
					txt_e = NULL;
				} else {
					txt_s = lend + 1;
					txt_e = xml_findend(txt_s, lname_s, lname_end - lname_s);
					if (txt_e == NULL) {
						txt_e = end;
					}
				}
				//printf("label:");
				//if(0==printstr(lname_s, lname_end - lname_s)){
				//	printf("   [%.10s]\n", lname_s);
				//}
				//printf("\n");
				*lend = 0;
				if (callback) {
					callback(p, lend - p, txt_s, txt_e - txt_s, user);
				}
				//xml_label_out(p, lend - p, txt_s, txt_e - txt_s);
			}
			p = lend;
			continue;
		}
		p++;
	}
}

#define XML_GetProfiles "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetProfiles xmlns=\"http://www.onvif.org/ver10/media/wsdl\"/>"\
	"</s:Body></s:Envelope>"

struct OnvifGetProfilesResContext {
	struct OnvifDeviceProfiles *profiles;
	int index;
};

static void _xml_GetProfilesResponse(const char *xmllabel, int llen,
		const char *txt, int tlen, void *user) {
	struct OnvifGetProfilesResContext *context =
			(struct OnvifGetProfilesResContext *) user;

	const char *label = strchr(xmllabel, ':');
	char *attrib = (char *) xmllabel;

	if (label) {
		label++;
	} else {
		label = xmllabel;
	}
	while (*attrib) {
		attrib++;
		if (*attrib == ' ')
			break;
	}
	if (attrib) {
		*attrib = 0;
		attrib++;
	}
	//printf("label:%s\n", label);
#define XML_LABEL_Profiles  "Profiles"
#define XML_LABEL_Bounds	"Bounds"
#define XML_LABEL_Encoding  "Encoding"
#define XML_LABEL_Name		"Name"

	if (strcasecmp(XML_LABEL_Profiles, label) == 0) {
		//printf("label:%s,attrib:%s\n", label, attrib);
		if (strstr(attrib, "token=")) {
			char token[100];
			sscanf(attrib, "token=%*c%[^\"\']", token);
			if (context->index < 5) {
				strcpy(context->profiles[context->index].token, token);
				context->index++;
			}
		}
	}
	if (strcasecmp(XML_LABEL_Bounds, label) == 0) {
		int width = 0, height = 0;
		if (strstr(attrib, "width")) {
			attrib = strstr(attrib, "width");
		}
		sscanf(attrib, "width=%*c%d%*c height=%*c%d", &width, &height);
		if (context->index > 0 && context->index < 5) {
			context->profiles[context->index - 1].width = width;
			context->profiles[context->index - 1].height = height;
		}
	}
	if (strcasecmp(XML_LABEL_Encoding, label) == 0) {
		((char*) txt)[tlen] = 0;
		//printf("encode:%s\n", txt);
	}
	if (strcasecmp(XML_LABEL_Name, label) == 0) {
		((char*) txt)[tlen] = 0;
		//printf("name:%s\n", txt);
	}
}

int Onvif_MediaService_GetProfiles(const char *mediaUrl, const char *user,
		const char *pass, struct OnvifDeviceProfiles profiles[5]) {
	char xmlstr[10240];
	int ret = 0;
	xmlstr[0] = 0;

	struct OnvifGetProfilesResContext context;
	context.profiles = profiles;
	context.index = 0;

	ret = onvif_http_digest_post(mediaUrl, user, pass, XML_GetProfiles, xmlstr,
			sizeof(xmlstr));
	xml_label(xmlstr, _xml_GetProfilesResponse, &context);
	return ret;
}

#define XML_GetStreamUri "<?xml version=\"1.0\" encoding=\"utf-8\"?>"\
	"<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"\
	"<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"\
	"<GetStreamUri xmlns=\"http://www.onvif.org/ver10/media/wsdl\">"\
	"<StreamSetup>"\
	"<Stream xmlns=\"http://www.onvif.org/ver10/schema\">RTP-Unicast</Stream>"\
	"<Transport xmlns=\"http://www.onvif.org/ver10/schema\">"\
	"<Protocol>UDP</Protocol>"\
	"</Transport>"\
	"</StreamSetup>"\
	"<ProfileToken>%s</ProfileToken>"\
	"</GetStreamUri></s:Body>"\
	"</s:Envelope>"

static void xml_GetStreamUriResponse(const char *xmllabel, int llen,
		const char *txt, int tlen, void *user) {
	const char *label = strchr(xmllabel, ':');
	char *attrib = (char *) xmllabel;

	if (label) {
		label++;
	} else {
		label = xmllabel;
	}
	while (*attrib) {
		attrib++;
		if (*attrib == ' ')
			break;
	}
	if (attrib) {
		*attrib = 0;
		attrib++;
	}

	if (strcmp("Uri", label) == 0) {
		((char*) txt)[tlen] = 0;
		strncpy((char*) user, txt, 300);
	}
}

int Onvif_MediaServer_GetStreamUri(const char *url, const char *user,
		const char *pass, const char *ProfileToken, char uri[300]) {
	char xmlstr[10240];
	int ret = 0;
	xmlstr[0] = 0;
	char str[1024];

	memset(str, 0, sizeof(str));
	snprintf(str, sizeof(str) - 1, XML_GetStreamUri, ProfileToken);
	ret = onvif_http_digest_post(url, user, pass, str, xmlstr, sizeof(xmlstr));
	xml_label(xmlstr, xml_GetStreamUriResponse, uri);
	return ret;
}

void OnvifURI_Decode(const char *url, char *desc, int dlen) {
#define URL_HEAD_RTSP "rtsp://"
	const char *ptr = NULL;
	const char *end = desc + dlen;

	strncpy(desc, url, dlen);

	if (strncasecmp(url, URL_HEAD_RTSP, strlen(URL_HEAD_RTSP)) != 0)
		return;
	//https://blog.csdn.net/haydroid/article/details/46380069
	//去除&amp;
#define URL_AMP "amp;"
	ptr = url;

	while (desc < end && *ptr) {
		*desc++ = *ptr;
		if (*ptr == '&') {
			if (strncmp(ptr + 1, URL_AMP, strlen(URL_AMP)) == 0) {
				ptr += strlen(URL_AMP) + 1;
				continue;
			}
		}
		ptr++;
	}
	if (desc < end)
		*desc = 0;
}

void Onvif_HttpTest() {
	char *url = "http://192.168.0.150/onvif/device_service";
	char *user = "admin";
	char *pass = "admin";
	int ret;

	char mediaUrl[100];
	struct OnvifDeviceInformation info;
	memset(&info, 0, sizeof(info));

	ret = Onvif_GetDeviceInformation(url, user, pass, &info);
	printf("Ma:%s, Mo:%s, F:%s,S:%s,HID:%s\n", info.Manufacturer, info.Model,
			info.FirmwareVersion, info.SerialNumber, info.HardwareId);

	Onvif_GetCapabilities_Media(url, user, pass, mediaUrl);

	printf("mediaUrl:%s\n", mediaUrl);

	struct OnvifDeviceProfiles profiles[5];
	memset(profiles, 0, sizeof(profiles));
	Onvif_MediaService_GetProfiles(mediaUrl, user, pass, profiles);
	char uri[300];

	for (int i = 0; i < 5; i++) {
		if (strlen(profiles[i].token) == 0)
			break;
		printf("token=%s\n", profiles[i].token);
		memset(uri, 0, sizeof(uri));
		Onvif_MediaServer_GetStreamUri(mediaUrl, user, pass, profiles[i].token,
				uri);
		OnvifURI_Decode(uri, uri, 300);
		printf("uri:%s\n", uri);
	}
}
