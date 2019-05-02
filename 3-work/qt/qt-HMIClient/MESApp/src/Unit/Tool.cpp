/*
 * Tool.cpp
 *
 *  Created on: 2015年1月29日
 *      Author: cj
 */

#include "Tool.h"
#include <QtCore>
#include "MESLog.h"
#ifdef Q_WS_WIN
#include <windows.h>
#endif
#ifdef  Q_OS_LINUX   //for linux
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
QSharedMemory *shareMem;
int unit::Tool::WirelessSignal()
{
	int sig = 0;
#ifdef _WIN32
	QFile file("d:/wireless.txt");
#else
	QFile file("/proc/net/wireless");
#endif
	if (!file.open(QIODevice::ReadOnly)){
		logErr("wireless file open err.");
		return 1;
	}
	QTextStream stream(&file);
	QString line = stream.readLine();
	line = stream.readLine();
	line = stream.readLine();
	//取第三行
	//wlan0: 0000   98.   73.    0.       0      0      0      0      0        0
	line.trimmed();

	QStringList list = line.split(" ", QString::SkipEmptyParts);
	if (list.size() > 4)
	{
		QString vtr = list[3];
		vtr.remove('.');
		int v=vtr.toInt();

		//0->100
		//0->25 26-50 50-75 75-100
		// < -90 : No Signal
		// < -81 : Very Low
		// < -71 : Low
		// < -67 : Good
		// < -57 : Very Good
		if(v<20){
			sig=0;
		} else if(v<21){
			sig=1;
		} else if(v< 50){
			sig=2;
		} else if(v<80){
			sig=3;
		} else {
			sig=4;
		}
	}
	file.close();
	return sig;
}

bool unit::Tool::getAppState()
{
#ifndef _WIN32
	/*FILE * fp;
	char buffer[20];
	memset(buffer,'\0',sizeof(buffer));
	fp=popen("pidof App_ChiefMES.linux","r");
	fgets(buffer,sizeof(buffer),fp);
	printf("\r\npid:%s",buffer);
	pclose(fp);
	if(strlen(buffer) ==0)
		return true;*/
	const char filename[]  = "./lockfile";
	int fd = open (filename, O_WRONLY | O_CREAT , 0644);
	int flock = lockf(fd, F_TLOCK, 0 );
	if (fd == -1)
	{
		printf("open lockfile\n");
		return false;
	}
	//给文件加锁
	if (flock == -1)
	{
		printf("lock file error\n");
		return false;
	}

	return true;
#else
	shareMem = new QSharedMemory(QString("App_ChiefMES.linux"));
	if (shareMem->create(1)  && shareMem->error() != QSharedMemory::AlreadyExists)
	{
		return true;
	}

#endif
	return false;
}
