#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <net/if_arp.h>

int getifrnameip(const char* ifrname)
{
    int sock_get_ip;
    char ipaddr[50];

    struct sockaddr_in* sin;
    struct ifreq ifr_ip;

    if ((sock_get_ip = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket create failse...GetLocalIp!/n");
        return -1;
    }

    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_name, ifrname, sizeof(ifr_ip.ifr_name) - 1);

    if (ioctl(sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0) {
        close(sock_get_ip);
        return -1;
    }
    sin = (struct sockaddr_in*)&ifr_ip.ifr_addr;
    strcpy(ipaddr, inet_ntoa(sin->sin_addr));

    printf("local ip:%s \n", ipaddr);
    close(sock_get_ip);
    return 0;
}

int main(int argc, char const* argv[])
{
    struct ifaddrs* ifAddrStruct = NULL;

    getifrnameip("eth0");
    printf("====================\n");

    if (getifaddrs(&ifAddrStruct) != 0) {
        perror("...");
        return -1;
    }
    struct ifaddrs* iter = ifAddrStruct;

    while (iter != NULL) {
        if (iter->ifa_addr->sa_family == AF_INET) { //if ip4
            // is a valid IP4 Address

            struct in_addr ip = ((struct sockaddr_in*)iter->ifa_addr)->sin_addr;

            printf("%s:%s\n", iter->ifa_name, inet_ntoa(ip));
        }
        if (iter->ifa_addr->sa_family == AF_INET6) { // check it is ip6

            /* deal ip6 addr */
            void* tmpaddrptr = &((struct sockaddr_in*)iter->ifa_addr)->sin_addr;
            char addressbuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpaddrptr, addressbuffer, INET6_ADDRSTRLEN);
            printf("\t%s\n", addressbuffer);
        }
        iter = iter->ifa_next;
    }
    //releas the struct
    freeifaddrs(ifAddrStruct);
    return 0;
}
