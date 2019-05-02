/*
 * Backlight.cpp
 *
 *  Created on: 2015年6月4日
 *      Author: cj
 */

#include "Backlight.h"
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#endif

Backlight::Backlight()
{
	// TODO Auto-generated constructor stub

}

Backlight::~Backlight()
{
	// TODO Auto-generated destructor stub
}


void Backlight::closeScreen()
{
	char buf[20];
#ifndef _WIN32
	int fd = ::open("/sys/class/backlight/pwm-backlight/brightness",O_RDWR|O_NONBLOCK);
	sprintf(buf,"%d",0);
	::write(fd,buf,sizeof(buf));
	::close(fd);
#endif
}

void Backlight::lightScren()
{
	#ifndef _WIN32
	char buf[20];
	int fd = ::open("/sys/class/backlight/pwm-backlight/brightness",O_RDWR|O_NONBLOCK);
	sprintf(buf,"%d",80);
	::write(fd,buf,sizeof(buf));
	::close(fd);
	#endif
}
