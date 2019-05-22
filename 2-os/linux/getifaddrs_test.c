#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char const* argv[])
{
    struct ifaddrs* ifAddrStruct = NULL;
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
