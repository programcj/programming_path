#include "OnvifDevice.hh"
#include <stdlib.h>

namespace onvif
{

    int urldecode(const char *url, struct URLInfo *urlinfo)
    {
        const char *ptr = url, *pos;
        const char *pend;
        size_t len = strlen(url);
        int flagauth = 0;

        ptr = strstr(url, "://");
        if (ptr == NULL)
            return -1;
        ptr += 3;
        strncpy(urlinfo->prefix, url, ptr - url);

        pend = strchr(ptr, '/');
        if (pend == NULL)
            pend = url + len;
        //寻找  username:password@host:port

        const char *pname_end = strchr(ptr, ':');
        const char *phost_begin;

        for (phost_begin = pend; phost_begin > ptr; phost_begin--)
        {
            if (*phost_begin == '@')
                break;
        }

        if (*phost_begin == '@' && pname_end < pend)
        {
            flagauth = 1;
            strncpy(urlinfo->useranme, ptr, pname_end - ptr);
            ptr = pname_end + 1;
            strncpy(urlinfo->password, ptr, phost_begin - ptr);
            ptr = phost_begin + 1;
        }
        //
        if (ptr >= pend)
        {
            printf("[%s]unknow host port\n", url);
            return -1;
        }
        //host and port;
        for (pos = ptr; pos < pend; pos++)
            if (*pos == ':')
                break;
        strncpy(urlinfo->host, ptr, pos - ptr);
        if (*pos == ':')
            ptr = pos + 1;
        else
            ptr = pos;

        if (ptr < pend)
        {
            urlinfo->port = atoi(ptr);
            ptr = pend;
        }
        else
        {
            if (strcmp(urlinfo->prefix, "http://") == 0)
                urlinfo->port = 80;
            if (strcmp(urlinfo->prefix, "https://") == 0)
                urlinfo->port = 443;
        }
        if (ptr)
            urlinfo->path = ptr;
        else
            urlinfo->path = "";
        return 0;
    }

    OnvifDevice::OnvifDevice(const char* uri, const char* username, const char* password, bool replaceipflag)
    {
        if (strncasecmp(uri, "http", strlen("http")) != 0) {
            snprintf(this->uri, sizeof(this->uri), "http://%s/onvif/device_service", uri);
        }
        else {
            strcpy(this->uri, uri);
        }

        strcpy(this->username, username);
        strcpy(this->password, password);
        ipreplaceflag = replaceipflag;

        char* url = strchr(this->uri, ' ');
        if (url)
            *url = 0;

        memset(&devInfo, 0, sizeof(devInfo));
        memset(mediaUrl, 0, sizeof(mediaUrl));
        memset(stat, 0, sizeof(stat));
        profiles = NULL;
        profsize = 0;
    }

    OnvifDevice::~OnvifDevice()
    {
        if (profiles)
            free(profiles);
    }

}