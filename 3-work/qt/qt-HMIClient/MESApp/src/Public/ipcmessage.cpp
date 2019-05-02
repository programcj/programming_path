/*
 * ipcmessage.cpp
 *
 *  Created on: 2015年6月5日
 *      Author: cj
 */

#include "ipcmessage.h"

IpcMessage::IpcMessage()
{
	// TODO Auto-generated constructor stub

}

IpcMessage::~IpcMessage()
{
	// TODO Auto-generated destructor stub
}

bool IpcMessage::sendMsgRestartApp()
{
#ifndef _WIN32
    key_t key = 3344;
    int msgid;
    msg_info msg;
	 //获取键值
	key=ftok("/etc/msg.tmp",1);
	if(-1 == key)
	{
		return false;
	}
   //打开或创建一个消息队列
	msgid=msgget(key,0);
	msg.type =1;
	msg.msgdata = RESET_MAIN_SOFTWARE;
	//发送消息
	int ret=msgsnd(msgid,&msg,sizeof(msgbuf),0);
	if(-1 == ret)
	{
	   return false;
	}
#endif
	return true;
}

bool IpcMessage::sendMsgRunNewApp()
{
#ifndef _WIN32
    key_t key = 3344;
    int msgid;
    msg_info msg;
//获取键值
	key=ftok("/etc/msg.tmp",1);
	if(-1 == key)
	{
		return false;
	}
   //打开或创建一个消息队列
	msgid=msgget(key, 0);
	msg.type =1;
	msg.msgdata = UPDATA_MAIN_SOFTWARE;
	//发送消息
	int ret=msgsnd(msgid,&msg,sizeof(msgbuf),0);
	if(-1 == ret)
	{
	   return false;
	}
#endif
	return true;


}
