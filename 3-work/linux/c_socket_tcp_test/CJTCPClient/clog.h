/*
 * clog.h
 *
 *  Created on: 2017年3月25日
 *      Author: cj
 */

#ifndef SRC_CLOG_H_
#define SRC_CLOG_H_

//日志极别
typedef enum log_level {
	L_ALL = 0xFF, L_DEBUG = 0x01, L_WRING = 0x02, L_INFO = 0x04, L_ERR = 0x08
} LOG_LEVEL;

extern int log_setconsole(int fd);

//日志输出信息: 级别，输出文件名:__FUNCTION__, 功能函数:__FUNCTION__, 行:__LINE__，输出格式串
extern int log_printf(LOG_LEVEL level, const char *file, const char *function,
		int line, const char *format, ...);

extern int log_printf_hex(LOG_LEVEL _level, const char *file,
		const char *function, int line,
		const void *data, int dlen,const char *format, ...);

#define log_i(format,...) log_printf(L_INFO,__FILE__,__FUNCTION__,__LINE__,format, ##__VA_ARGS__)
#define log_d(format,...) log_printf(L_DEBUG,__FILE__,__FUNCTION__,__LINE__,format, ##__VA_ARGS__)
#define log_e(format,...) log_printf(L_ERR,__FILE__,__FUNCTION__,__LINE__,format, ##__VA_ARGS__)
#define log_w(format,...) log_printf(L_WRING,__FILE__,__FUNCTION__,__LINE__,format, ##__VA_ARGS__)

#define log_hex(data,dlen,format,...) log_printf_hex(L_DEBUG,__FILE__,__FUNCTION__,__LINE__,data,dlen, format, ##__VA_ARGS__)

#ifdef CONFIG_CLOG_BASIC
#define log_d(format,...)  do { \
		printf("%s,%s,%d:"format"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  fflush(stdout); } while(0);
#endif

#endif /* SRC_CLOG_H_ */
