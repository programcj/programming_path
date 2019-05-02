/*
 * ipcmessage.h
 *
 *  Created on: 2015年6月5日
 *      Author: cj
 */

#ifndef IPCMESSAGE_H_
#define IPCMESSAGE_H_
#include <stdio.h>
#ifndef _WIN32
#include <sys/ipc.h>
#include <sys/msg.h>
#endif
#include <stdlib.h>
#include <string.h>

typedef struct aa
{
    long type;
    int msgdata;
}msg_info;

#define RESET_MAIN_SOFTWARE   0x00AA  //重启程序
#define UPDATA_MAIN_SOFTWARE  0x00BB  //更新程序

class IpcMessage
{
public:
	IpcMessage();
	virtual ~IpcMessage();

	static bool sendMsgRestartApp(); //重启程序

	static bool sendMsgRunNewApp();  //当下载新的程序时，运行新的程序


};

#endif /* IPCMESSAGE_H_ */
