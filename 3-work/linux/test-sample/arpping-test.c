#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_arp.h>
#include <sys/ioctl.h>

#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <sys/poll.h>

struct __attribute__((__packed__)) arpreqipv4
{
    unsigned char srcmac[6];
    struct in_addr srcaddr;
    unsigned char dstmac[6];
    struct in_addr dstaddr;
};

static int arp_sendpackreq4(int sock_fd,
                            struct in_addr *src_addr,
                            struct in_addr *dst_addr,
                            uint8_t *srcmac,
                            uint8_t *dstmac,
                            struct sockaddr_ll *addr)
{
    int err;
    unsigned char buf[256];
    struct arphdr *ah = (struct arphdr *)buf;
    struct arpreqipv4 *p = (struct arpreqipv4 *)(ah + 1);

    ah->ar_hrd = htons(ARPHRD_ETHER);
    ah->ar_pro = htons(ETH_P_IP);
    ah->ar_hln = 6; //ME->sll_halen;
    ah->ar_pln = 4;
    ah->ar_op = htons(ARPOP_REQUEST); // : htons(ARPOP_REPLY);

    memcpy(p->srcmac, srcmac, 6);
    memcpy(&p->srcaddr, src_addr, 4);
    memcpy(p->dstmac, dstmac, 6);
    memcpy(&p->dstaddr, dst_addr, 4);
    memcpy(p + 1, "hello hello", 11);

    err = sendto(sock_fd, buf, sizeof(struct arphdr) + sizeof(struct arpreqipv4) + 11,
                 0, (struct sockaddr *)addr, sizeof(*addr));

    printf("send arp: %d, srcmac:%X:%X\n", err,
           ntohl(*((uint32_t *)srcmac)), ntohs(*(uint16_t *)(srcmac + 4)));
    if (err <= 0)
    {
        perror("err....");
    }
    return err;
}

int arp_getmac(const char *ifrname, const char *host, void *mac)
{
    int sock_fd = 0;
    int ret;
    sock_fd = socket(AF_PACKET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
        return -1;

    struct sockaddr_ll me;
    struct in_addr src;
    struct in_addr dst;

    //inet_pton(AF_INET, "192.168.8.106", &src);
    inet_pton(AF_INET, host, &dst);

    memset(&me, 0, sizeof(me));
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, ifrname);
        ret = ioctl(sock_fd, SIOCGIFINDEX, (char *)&ifr);
        if (ret)
        {
            perror("ifindex");
            goto __quit;
        }

        me.sll_ifindex = ifr.ifr_ifindex;

        ret = ioctl(sock_fd, SIOCGIFADDR, &ifr);
        if (ret)
        {
            perror("get addr");
            goto __quit;
        }
        src = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

        ret = ioctl(sock_fd, SIOCGIFHWADDR, &ifr);
        if (ret)
        {
            perror("get hw addr");
            goto __quit;
        }
        memcpy(me.sll_addr, ifr.ifr_hwaddr.sa_data, 6);
    }
    me.sll_family = AF_PACKET;
    me.sll_protocol = htons(ETH_P_ARP);
    me.sll_halen = 6;
    ret = bind(sock_fd, (struct sockaddr *)&me, sizeof(me));
    if (ret)
    {
        perror("bind");
        goto __quit;
    }

    uint8_t packet[300];
    printf("src=%s\n", inet_ntoa(src));
    printf("dst=%s\n", inet_ntoa(dst));

    time_t cur = 0;
    struct pollfd fds;
    fds.fd = sock_fd;
    fds.events = POLLIN;

    if (src.s_addr == dst.s_addr)
    {
        memcpy(mac, me.sll_addr, 6);
        goto __quit;
    }
    int reqcount = 0;
    while (reqcount < 3)
    {
        if (time(NULL) - cur > 2)
        {
            struct sockaddr_ll he;
            he = me;
            he.sll_halen = 6;
            memset(he.sll_addr, 0xFF, he.sll_halen);
            unsigned char dscmac[6];
            memset(dscmac, 0, 6);
            arp_sendpackreq4(sock_fd, &src, &dst, me.sll_addr, dscmac, &he);
            cur = time(NULL);
            reqcount++;
            ret = -1;
        }

        struct sockaddr_ll from;
        socklen_t alen = sizeof(from);
        int cc;
        ret = poll(&fds, 1, 200);
        if (ret < 0)
            break;
        if (ret == 0)
        {
            ret = -1;
            continue;
        }

        cc = recvfrom(sock_fd, packet, 300, 0, (struct sockaddr *)&from, &alen);
        if (cc <= 0)
        {
            ret = -1;
            continue;
        }

        {
            struct arphdr *ah = (struct arphdr *)packet;
            struct arpreqipv4 *ainfo = (typeof(ainfo))(ah + 1);

            printf("arp %d, %s->", ntohs(ah->ar_op), inet_ntoa(ainfo->srcaddr));
            printf("%s", inet_ntoa(ainfo->dstaddr));
            printf("\n");

            if (ainfo->srcaddr.s_addr == dst.s_addr)
            {
                if (ntohs(ah->ar_op) == ARPOP_REPLY || ntohs(ah->ar_op) == ARPOP_REQUEST)
                {
                    memcpy(mac, ainfo->srcmac, 6);
                    ret = 0;
                    break;
                }
            }
            ret = -1;
        }
        for (int i = 0; i < cc; i++)
        {
            printf("%02X ", packet[i]);
        }
        printf("\n");
    }
__quit:
    if (sock_fd != -1)
        close(sock_fd);
    return ret;
}

int main(int argc, const char **argv)
{
    const char *device = "wlp2s0";
    unsigned char mac[8];
    int ret;

    memset(mac, 0, sizeof(mac));
    ret = arp_getmac(device, argv[1], mac);

    printf("ret=%d, mac:%lX:%X\n", ret, ntohl(*(uint32_t *)mac), ntohs(*(uint16_t *)(mac + 4)));
    return 0;
}