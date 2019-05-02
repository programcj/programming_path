/*
 * GPIO.cpp
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */

#include "GPIO.h"

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

GPIO GPIO::instance;

GPIO &
GPIO::getInstance ()
{
  return instance;
}

bool
GPIO::Write (quint16 port, quint16 value)
{
#ifndef _WIN32
  return write (m_GpioHandle, &value, port);
#endif
  return false;
}

GPIO::GPIO ()
{
  m_GpioHandle = 0;
#ifndef _WIN32
  m_GpioHandle = open (GPIO_NAME, O_RDWR, 0);
  if (m_GpioHandle != 0)
   {
  /* ioctl(m_GpioHandle, GPIO_OUT, PORT_OUT_1);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_OUT_2);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_OUT_3);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_OUT_4);*/

   ioctl(m_GpioHandle, GPIO_OUT, PORT_USB_POWER);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_LED_IN_1);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_LED_IN_2);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_LED_IN_3);
   ioctl(m_GpioHandle, GPIO_OUT, PORT_LED_IN_4);

   ioctl(m_GpioHandle, GPIO_INPUT, PORT_INPUT_1);
   ioctl(m_GpioHandle, GPIO_INPUT, PORT_INPUT_2);
   ioctl(m_GpioHandle, GPIO_INPUT, PORT_INPUT_3);
   ioctl(m_GpioHandle, GPIO_INPUT, PORT_INPUT_4);


   }
#endif
}


bool
GPIO::IoRead (quint16 port, quint16& value)
{

#ifndef _WIN32
   quint16 readdata =0;
   read (m_GpioHandle, &readdata, port);
  // printf("read port:%d, value:%d\n",port,readdata);
   value =readdata;
#endif
  return true;
}

GPIO::~GPIO ()
{
#ifndef _WIN32
  close (m_GpioHandle);
#endif
}
