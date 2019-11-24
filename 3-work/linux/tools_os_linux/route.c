#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include "route.h"

int os_route_get(struct os_route** infos, int* count)
{
    FILE* fp = fopen("/proc/net/route", "r");
    *infos = NULL;
    *count = 0;
    int line = -1;

    if (!fp)
        return -1;

    while (!feof(fp)) {
        fscanf(fp, "%*[^\n]\n");
        line++;
    }

    fseek(fp, 0, SEEK_SET);

    if (line > 0) {
        fscanf(fp, "%*[^\n]\n");

        char devname[64], flags[16];
        unsigned long d, g, m;
        int flgs, ref, use, metric, mtu, win, ir;
        int r;

        struct os_route* item = NULL;
        *infos = (struct os_route*)calloc(sizeof(struct os_route), line);
        if(!*infos)
            goto _reterr;

        *count = line;
        item = *infos;
        while (1) {
            r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                &mtu, &win, &ir);
            if (r != 11) {
                if (feof(fp) && r < 0)
                    break;
            }
            if (r < 5)
                continue;

            if (!(flgs & RTF_UP))
                continue;

            strcpy(item->devname, devname);
            item->dsc.s_addr = d;
            item->gw.s_addr = g;
            item->mask.s_addr = m;
            item->flgs_rtf = flgs;
            item->metric= metric;
            item++;
        }
    }

    fclose(fp);
    fp = NULL;
    printf("Line:%d\n", line);
    return 0;

_reterr:
    if (fp)
        fclose(fp);
    return -1;
}

#if CONFIG_DEBUG_ROUTE
int main(int argc, char const* argv[])
{
    struct os_route* infos = NULL;
    int count = 0;

    os_route_get(&infos, &count);

    for (int i = 0; i < count; i++) {
        
        printf("%s ", infos[i].devname);
        printf("\t%s", inet_ntoa(infos[i].dsc));
        printf(" \t%s", inet_ntoa(infos[i].gw));
        printf(" \t%s", inet_ntoa(infos[i].mask));
        printf(" \t%x", infos[i].flgs_rtf);
        printf(" \t%d", infos[i].metric);
        printf("\n");
    }

    if (infos)
        free(infos);

    return 0;
}
#endif