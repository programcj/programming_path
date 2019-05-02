/*
 * buzzer.cpp
 *
 *  Created on: 2015年3月20日
 *      Author: cj
 */

#include "buzzer.h"
#include <QDebug>
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
#define PWM_IOCTL_SET_FREQ	1
#define PWM_IOCTL_STOP		0

Buzzer::Buzzer ()
{
  // TODO Auto-generated constructor stub
#ifndef _WIN32
  bzfd = open("/dev/buzzer", O_RDWR);
  if (bzfd < 0)
   {
        qDebug("open buzzer device error");
   }
#endif
}

Buzzer::~Buzzer ()
{
  // TODO Auto-generated destructor stub
#ifndef _WIN32
  close(bzfd);
#endif
}

void Buzzer::startBuzzer()
{
#ifndef _WIN32
  int ret = ioctl(bzfd, PWM_IOCTL_SET_FREQ, 5000);
  if(ret < 0)
  {
      qDebug("set the frequency of the buzzer error!");
  }
  usleep(100000);
  ret = ioctl(bzfd, PWM_IOCTL_SET_FREQ,0);
  if(ret < 0)
  {
        qDebug("set the frequency of the buzzer error!");
  }
#endif
}
