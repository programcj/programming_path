/**
 * 20190430 
 * @auth: cj
 * @note:
 *      字符串扩展功能,从tools中分割出来(衍生)
 */

#ifndef STRINGEX_C

#define STRINGEX_C

#ifdef __cplusplus
extern "C" {
#endif

//Hex 字符串转换成 到 data 中
int stringhex2data(const char *hexstring, void *data, int dlen);

//把bin数据转换成hex字符串
//char **buf hex_buf; 转换成的HEX字符串存放位置, 需要手动 free
void stringhex_from_data(char **hex_buf, const void *hex_data, int len);

//字符串复制,判断src字符串是否为空
char* strcpyex(char *desc, const char *src);

//字符串复制,判断src字符串是否为空
char* strncpyex(char *desc, const char *src, int maxsiz);

//字符串分割，每个串均在spptr指针中
//@spptr min size >=2, len min >=1
//len为1,如果有分割，则spptr[0] spptr[1] 始终存在 spptr[len]=为下一个串
//return <0 错误没有分割; >0 分割个数
////////////////////////////////////////////
//char buff1[] = "name:admin:34324:312";
// char *ptr[5] = {0};
// int len=strspilts(buff1, ":", ptr, 5); ->len=4, ptr[0]="name"..
// int len=strspilts(buff1, ":", ptr, 1); ->len=1, ptr[0]="name", ptr[1]="admin..."
int strspilts(char *buff, char *spchr, char **spptr, int len);

/**
 * 寻找子字符串,使用通配符方法
 * substr:
 *   %?:匹配一个字符
 *   %*:匹配n个字符
 *   %%:匹配%字符
 */
char *strstr2(const char *str, const char *substr);

//比较字符串开始串 toffset 位置开始，不会校验 strlen(str)
//@toffset 开始位置
//return 0 表示相等
int strcmp_startwith(const char *str, const char *prefix, int toffset);

//比较字符串尾串  suffix
//return 0 表示相等
int strcmp_endwith(const char *str, const char *suffix);

#ifdef __cplusplus
}
#endif


#endif