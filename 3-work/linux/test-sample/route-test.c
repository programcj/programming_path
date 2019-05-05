#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t get_route4_defaultgw()
{
    char line[500];
    uint32_t defgw = 0;
    FILE* fp = fopen("/proc/net/route", "r");
    if (!fp)
        return defgw;
    char iface[30];
    struct in_addr dscip;
    struct in_addr gw;
    struct in_addr mask;

    int ret = 0;

    //eth0	00000000	0100A8C0	0003	0	0	100	000000000	0	0
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        ret = sscanf(line, "%s %lx %lx %*d %*d %*d %*d %lx", iface, &dscip.s_addr,
            &gw.s_addr, &mask.s_addr);
        if (ret > 1) {
            if (dscip.s_addr == 0) //默认网关
            {
                defgw = gw.s_addr;
                break;
            }
        }
    }
    fclose(fp);
    return defgw;
}

void route4_print()
{
    char line[500];
    FILE* fp = fopen("/proc/net/route", "r");
    if (!fp)
        return;
    char iface[30];
    struct in_addr dscip;
    struct in_addr gw;
    struct in_addr mask;

    int ret = 0;

    //eth0	00000000	0100A8C0	0003	0	0	100	000000000	0	0
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        ret = sscanf(line, "%s %lx %lx %*d %*d %*d %*d %lx", iface, &dscip.s_addr,
            &gw.s_addr, &mask.s_addr);
        if (ret > 1) {
            if (dscip.s_addr == 0) //默认网关
                printf("*");
            printf("%-9s", iface);
            printf("%-15s ", inet_ntoa(dscip));
            printf("%-15s ", inet_ntoa(gw));
            printf("%-15s ", inet_ntoa(mask));
            printf("\n");
        }
    }
    fclose(fp);
}

int main(int argc, char const* argv[])
{
    route4_print();
    return 0;
}
