#ifndef _ROUTE_H_
#define _ROUTE_H_

#include <netinet/in.h>

#define RTF_UP 0x0001 /* route usable                 */
#define RTF_GATEWAY 0x0002 /* destination is a gateway     */
#define RTF_HOST 0x0004 /* host entry (net otherwise)   */
#define RTF_REINSTATE 0x0008 /* reinstate route after tmout  */
#define RTF_DYNAMIC 0x0010 /* created dyn. (by redirect)   */
#define RTF_MODIFIED 0x0020 /* modified dyn. (by redirect)  */
#define RTF_MTU 0x0040 /* specific MTU for this route  */

struct os_route {
    char devname[64];
    struct in_addr dsc;
    struct in_addr gw;
    struct in_addr mask;
    int flgs_rtf;
    int metric; //跃点
};

int os_route_get(struct os_route** infos, int* count);

#endif