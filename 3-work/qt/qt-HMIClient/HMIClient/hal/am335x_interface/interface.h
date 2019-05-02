#ifndef __INTERFACE__H__
#define __INTERFACE__H__

#ifdef __cplusplus
extern "C" {
#endif
#ifndef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <errno.h>

#include <linux/tty.h>
#include <linux/types.h>
#include <sys/ioctl.h>

#endif

#define AM335X_COM_CONSOLE "/dev/ttyO0"
#define AM335X_COM_232   "/dev/ttyO2"
#define AM335X_COM_485   "/dev/ttyO1"
#define AM335X_COM_RFID  "/dev/ttyO3"

#define am335_serial_232(__oflag)  open(AM335X_COM_232,__oflag)
#define am335_serial_485(__oflag)  open(AM335X_COM_485,__oflag);
#define am335_serial_rfid(__oflag) open(AM335X_COM_RFID,__oflag);

int file_exists(const char *file);

void gpio_init_out();

void gpio_init_input();

/**
 * @brief gpio_set_value
 * @param outindex 0-3
 * @param value
 * @return
 */
int gpio_set_value(unsigned int outindex,int value);

int gpio_read(const char *gpio,int *value);

int gpio_out_read(int outindex);

/**
 * @brief gpio_input_read
 * @param inputindex 0-3
 * @param value
 * @return default 1
 */
int gpio_input_read(unsigned int inputindex,int *value);

/**
 * @brief gpio_input_name
 * @param index 0-3
 * @return
 */
const char *gpio_input_name(unsigned int index);

/**
 * @brief led_set
 * @param index 0-4
 * @param value
 * @return
 */
int led_set(int index,int value);

/**
 * @brief led_value
 * @param index 0-4
 * @return
 */
int led_value(int index);


#ifdef __cplusplus
}
#endif


#endif 
