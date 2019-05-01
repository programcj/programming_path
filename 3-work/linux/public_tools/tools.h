/*
 * tools.h
 *
 *  Created on: 2015年12月1日
 *      Author: cj
 *      Email:593184971@qq.com
 */

#ifndef COMMON_TOOLS_H_
#define COMMON_TOOLS_H_

#define BYTE_2_SHORT(msb, lsb) ((0x00FF & msb) << 8 | (0x00FF & lsb))

//文件是否存在,存在返回>0;  不存在返回0
extern int tools_file_exists(const char *fileName);

/*是否是一个目录*/
extern int tools_file_isdir(const char *file_path);

/*是否是一个常规文件.*/
extern int tools_file_isreg(const char *file_path);

//取文件的大小
extern unsigned long tools_file_get_size(const char *filePath);

//从文件中读取size大小的数据到data中
extern int tools_file_read(const char *fileName, int index, unsigned char *data, int size);
//文件的crc16校验
extern unsigned short tools_file_crc16(const char *fileName);
//crc16校验字符串
extern unsigned short tools_crc16(unsigned char *buf, int length);

//32位md5sum计算
extern int tools_md5sum_calc(const char *filePath, char md5str[33]);

extern int tools_readcmdline(const char *cmdline, char *buff, int maxsize);

//读取文件到buf, 外部buf不为空时须要free内存, return 0 success, -1 err
extern int tools_read_file(const char *fileName, char **buf);

extern int tools_read_file_tobuff(const char *filename, char *buff, int maxsize);

//文件写入一个字符串 w+
extern int tools_write_file(const char *fileName, const char *buf);

extern void tools_uuid(char uuid[33]);

#include <stdint.h>

extern uint64_t tools_current_time_millis();
//系统运行时间
extern uint64_t tools_os_runtime(int secflag);

extern void tools_timeget_local(char *timestring);

/**
 * 时:分:秒
 * %02d:%02d:%02d
 */
extern void tools_timeget_local_tim(char tim[9]);

extern void tools_timeget_otc(char timestring[30]);

//以当期时间为准，从字符串读时间[yyy?mm?dd h:m:s][yyy?mm?dd][h?m?s]
extern int tools_timevalcur_fromstr(struct timeval *ptv, const char *tmstr);


/*进度 位置百分比  x/y=?/Z  ?= x/y*Z */
#define tools_progress_index(cur, max, progsum) ((cur / max) * progsum)

/*配置文件读取功能: name=value #aaaa*/
extern int tools_config_file_getstr(const char *file_path, const char *name, char *valuestr, int vlen);

/*IPV4地址检测功能*/
extern int tools_ipv4strcheck(const char *ipstring, uint32_t *ipv4);

/*获取时间转数字[秒0]
 * @usecnumber: 微妙取开始位置个数(3,4,5)
*/
static uint64_t tools_gettimeofday2int(int usecnumber);


#endif /* COMMON_UTIL_H_ */
