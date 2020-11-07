/*
 * log.h
 *
 *  Created on: 2019年11月6日
 *      Author: cc
 *
 *                .-~~~~~~~~~-._       _.-~~~~~~~~~-.
 *            __.'              ~.   .~              `.__
 *          .'//                  \./                  \\`.
 *        .'//                     |                     \\`.
 *      .'// .-~"""""""~~~~-._     |     _,-~~~~"""""""~-. \\`.
 *    .'//.-"                 `-.  |  .-'                 "-.\\`.
 *  .'//______.============-..   \ | /   ..-============.______\\`.
 *.'______________________________\|/______________________________`.
 *.'_________________________________________________________________`.
 * 
 * 请注意编码格式
 */

#ifndef SRC_INC_LIBUTILIY_LOG_H_
#define SRC_INC_LIBUTILIY_LOG_H_

void log_printf(const char *level, const char *module_name, const char *file,
			const char *function, int line, const char *format, ...);

#define log_i(module_name, format,...) log_printf("Info", module_name,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define log_w(module_name, format,...) log_printf("Wan", module_name,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define log_e(module_name, format,...) log_printf("Err", module_name,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define log_d(module_name, format,...) log_printf("Debug", module_name,  __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)

#endif /* SRC_INC_LIBUTILIY_LOG_H_ */
