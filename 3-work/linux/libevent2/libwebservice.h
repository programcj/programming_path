/*
 * libweb.h
 *
 *  Created on: 2021年2月26日
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

#ifndef SRC_LIBWEBSERVICE_LIBWEBSERVICE_H_
#define SRC_LIBWEBSERVICE_LIBWEBSERVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

	struct evhttp_request;

	//#include "evhttp_ex.h"

		//初始化端口号
	void webservice_init(int web_port);

	//获取端口号
	int webservice_getport();

	//内部会创建线程
	void webservice_start();
	void webservice_stop();

	//注册API调用
	void webserver_regedit_api(const char* path, void (*fun)(struct evhttp_request* req));

	//获取API
	void webservice_apis(void (*fun)(const char* name, void* ctx), void* ctx);

	//不会创建线程
	void webservice_loop();
	

#ifdef __cplusplus
}
#endif

#endif /* SRC_LIBWEBSERVICE_LIBWEBSERVICE_H_ */
