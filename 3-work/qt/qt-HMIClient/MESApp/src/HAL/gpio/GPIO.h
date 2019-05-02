/*
 * GPIO.h
 *
 *  Created on: 2015年2月10日
 *      Author: cj
 */
#include <QtGlobal>
#ifndef GPIO_H_
#define GPIO_H_

#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#define GPIO_NAME 				"/dev/GPIO_Dev0"
#define GPIO_INPUT 				0
#define GPIO_OUT 				1
//#define PORT_WATCHDOG
#define PORT_USB_POWER      GPIO_TO_PIN(3,21)

#define PORT_OUT_1			GPIO_TO_PIN(1,21)
#define PORT_OUT_2			GPIO_TO_PIN(1,16)
#define PORT_OUT_3			GPIO_TO_PIN(1,28)
#define PORT_OUT_4			GPIO_TO_PIN(0,20)

#define PORT_LED_IN_1			GPIO_TO_PIN(1,20)
#define PORT_LED_IN_2			GPIO_TO_PIN(1,22)
#define PORT_LED_IN_3			GPIO_TO_PIN(1,23)
#define PORT_LED_IN_4			GPIO_TO_PIN(1,27)

#define PORT_INPUT_1			GPIO_TO_PIN(1,17)
#define PORT_INPUT_2			GPIO_TO_PIN(1,24)
#define PORT_INPUT_3			GPIO_TO_PIN(1,25)
#define PORT_INPUT_4			GPIO_TO_PIN(1,26)
#define LOGIC_HIGH				1
#define LOGIC_LOW				0
//#define PORT_BELL

/*
 * GPIO输入输出端口
 */
class GPIO
{
  static GPIO instance;
  int m_GpioHandle;
public:
  static GPIO &  getInstance ();
  GPIO ();

  bool
  Write (quint16 port, quint16 value);
  bool
  IoRead (quint16 port, quint16 &value);



  ~GPIO ();
};
#endif /* GPIO_H_ */
