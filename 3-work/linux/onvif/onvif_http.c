#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libdigest/src/client.h"

#include <curl/curl.h>

static int http_content_point(const char* txt)
{
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

static int http_auth_digest(const char* dsc_disget, char* resultstr, int reslen,
    const char* username, const char* password)
{
    //https://github.com/jacketizer/libdigest
    digest_t d;

    if (-1 == digest_is_digest(dsc_disget)) {
        fprintf(stderr, "Could not digest_is_digest!\n");
        return -1;
    }

    digest_init(&d);

    digest_client_parse(&d, dsc_disget);

    digest_set_attr(&d, D_ATTR_USERNAME, (digest_attr_value_t)username);
    digest_set_attr(&d, D_ATTR_PASSWORD, (digest_attr_value_t)password);
    digest_set_attr(&d, D_ATTR_URI, (digest_attr_value_t) "/");

    char* cnonce = "400616322553302B623F0A0C514B0543";
    digest_set_attr(&d, D_ATTR_CNONCE, (digest_attr_value_t)cnonce);

    digest_set_attr(&d, D_ATTR_ALGORITHM, (digest_attr_value_t)1);
    digest_set_attr(&d, D_ATTR_METHOD,
        (digest_attr_value_t)DIGEST_METHOD_POST);

    if (-1 == digest_client_generate_header(&d, resultstr, reslen)) {
        fprintf(stderr, "Could not build the Authorization header!\n");
    }
    return 0;
}

static size_t _http_curl_wheader(char* buffer, size_t size, //大小
    size_t nitems, //哪一块
    void* out)
{
    strncat(out, buffer, size * nitems);
    return size * nitems;
}

static size_t _http_curl_wcontent(char* buffer, size_t size, //大小
    size_t nitems, //哪一块
    void* out)
{
    strncat(out, buffer, size * nitems);
    return size * nitems;
}

static int http_post(const char* url, const char* authorization,
    const char* poststr, char* respstr, int resplen)
{
    CURL* curl;
    CURLcode res;
    struct curl_slist* http_header = NULL;

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
	//curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

    curl_easy_setopt(curl, CURLOPT_URL, url); //url地址
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, poststr); //post参数
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(poststr));

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _http_curl_wheader); //处理http头部
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, respstr); //http头回调数据

    //curl_easy_setopt(curl, CURLOPT_HEADER, 1);  //将响应头信息和相应体一起传给write_data

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _http_curl_wcontent); //处理http内容
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, respstr); //这是write_data的第四个参数值

    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, resplen - 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息

    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
    //curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/tmp/curlpost.cookie");

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); //接收数据时超时设置，如果10秒内数据未接收完，直接退出
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0); //设为false 下面才能设置进度响应函数

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
int onvif_http_digest_post(const char* url, const char* username,
    const char* password, const char* postStr, char* contentText,
    int contentTextlen)
{

    char *authenticate401 = NULL, *ptr = NULL;
    char authorization[300];

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
	
	printf("authenticate401=%s\n", authenticate401);

    ret = http_auth_digest(authenticate401, authorization + namelen,
        sizeof(authorization) - namelen, username, password);

    contentText[0] = 0;

    ret = http_post(url, authorization, postStr, contentText, contentTextlen);
    return ret;
}

void test1()
{
    const char* url = "http://api.fanyi.baidu.com/api/trans/vip/translate";
    const char* postStr = "q=apple&from=en&to=zh&appid=2015063000000001&salt=1435660288&sign=f89f9594663708c1605f3d736d01d2d4";

    char buff[1024 * 10];
    int ret = 0;
    memset(buff, 0, sizeof(buff));

    ret = http_post(url, NULL, postStr, buff, sizeof(buff));
    usleep(100);
    printf("ret=%d\n", ret);
    printf("context=%s\n", buff);

    printf("==================================\n");

    const char digest_str[] = "Digest realm=\"test\", qop=\"auth\", nonce=\"9e9cb182c25b68148676a98cda86d501\"";

    char authstr[200];

    memset(authstr, 0, sizeof(authstr));
    ret = http_auth_digest(digest_str, authstr, sizeof(authstr), "admin",
        "password");

    printf("digest_str:%s\n", digest_str);
    printf("authstr:%s\n", authstr);

    {
        char content[1024 * 20]; //20k
        const char* postStr = "<onvif></onvif>";

        memset(content, 0, sizeof(content));
        ret = onvif_http_digest_post(url, "admin", "pass", postStr, content,
            sizeof(content) - 1);

        printf(">>>>>>>>>>>>>>ret=%d\n", ret);
        printf("content=%s\n", content);
    }
}

int main(int argc, char const* argv[])
{
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
        return -1;
    }
    int ret = 0;
    //test1();
    char resp[1024 * 100];
    const char* url = "http://192.168.0.150/onvif/device_service";
    const char* GetDeviceInformationStr = "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><GetDeviceInformation xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/></s:Body></s:Envelope>";

    ret = onvif_http_digest_post(url, "admin", "admin", GetDeviceInformationStr, resp,
        sizeof(resp));
    printf("ret=%d, %s\n", ret, resp);
    return 0;
}
