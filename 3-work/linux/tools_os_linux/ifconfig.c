#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/ioctl.h>

#define __USE_MISC
#include <net/if.h>
#include <arpa/inet.h>

void _print_bytes_scale(unsigned long long v)
{
    //unsigned long long u1;
    long double u;
    //unsigned long long u;

    static const char ex[][3] = {"B", "KB", "MB", "GB"};
    int i = 0;
    u = v;
    while (u >= 1024 && i < sizeof(ex) / sizeof(ex[0]))
    {
        u /= 1024;
        i++;
    }

    printf("%.2f %s", (double)u, ex[i]);
}

int os_ifconfig_print(const char *ifname)
{
    struct ifreq ifr;
    int skfd;
    int ret;
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd == -1)
        return -1;

    printf("%s:", ifname);
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    ret = ioctl(skfd, SIOCGIFFLAGS, &ifr);
    if (ret < 0)
        goto _end;
    printf(" flag:%X", ifr.ifr_flags);

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    ret = ioctl(skfd, SIOCGIFHWADDR, &ifr);
    if (ret < 0)
        goto _end;
    printf(" mac:%X:%X", ntohl(*((uint32_t *)ifr.ifr_hwaddr.sa_data)), ntohs(*((uint16_t *)(ifr.ifr_hwaddr.sa_data + 4))));
    printf(" family:%d", ifr.ifr_hwaddr.sa_family);

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    ret = ioctl(skfd, SIOCGIFTXQLEN, &ifr);
    if (ret < 0)
        goto _end;
    printf(" txqueue:%d", ifr.ifr_qlen);

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;

    ret = ioctl(skfd, SIOCGIFADDR, &ifr);
    if (!ret)
    {
        struct in_addr addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
        printf(" IP:%s", inet_ntoa(addr));

        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
        if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) >= 0)
        {
            addr = ((struct sockaddr_in *)&ifr.ifr_dstaddr)->sin_addr;
            printf(" DSTADDR:%s", inet_ntoa(addr));
        }

        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
        if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) >= 0)
        {
            addr = ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr;
            printf(" braddr:%s", inet_ntoa(addr));
        }

        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
        if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0)
        {
            addr = ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr;
            printf(" mask:%s", inet_ntoa(addr));
        }
    }
    {
#define _PATH_PROCNET_DEV "/proc/net/dev"
        char buf[512];
        FILE *fp = fopen(_PATH_PROCNET_DEV, "r");
        if (fp)
        {
            char name[128];
            char *p = NULL;
            fscanf(fp, "%*[^\n]\n");
            fscanf(fp, "%*[^\n]\n");
            while (fgets(buf, sizeof buf, fp))
            {
                memset(name, 0, sizeof(name));
                p = buf;
                while (*p && p < buf + sizeof(buf) && isspace(*p))
                    p++;
                ret = sscanf(p, "%[^:]", name);

                if (ret == 1)
                {
                    p = strchr(p, ':');
                    p++;
                    printf(">>>[%s] \n", name);
                    if (strcmp(name, ifname))
                        continue;

                    //bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
                    //  0        0      0    0    0     0          0         0      0       0       0    0    0     0       0     0
                    unsigned long long value;
                    int column = 0;
                    while (*p)
                    {
                        if (isdigit(*p))
                        {
                            value = atoll(p);

                            column++;
                            switch (column)
                            {
                            case 1:
                                printf("rx:%lld ", value);
                                _print_bytes_scale(value);
                                printf(" ");
                                break;
                            case 2:
                                printf("packets:%lld ", value);
                                break;
                            case 9:
                                printf("tx:%lld ", value);
                                
                                break;
                            case 10:
                                printf("packets:%lld", value);
                                break;
                            }

                            if (strchr(p, ' '))
                                p = strchr(p, ' ');
                        }

                        p++;
                    }
                    //sscanf(p, "%llu", &rx_bytes);
                    //__strtok_r()
                    //printf("rx:%lld ", rx_bytes);
                    break;
                }
            }
            fclose(fp);
        }
    }
_end:

    close(skfd);

    printf("\n");
    return ret;
}

#define CONFIG_TEST_DEBUG
#ifdef CONFIG_TEST_DEBUG
int main(int argc, char const *argv[])
{
    os_ifconfig_print("wlp2s0");
    return 0;
}

#endif