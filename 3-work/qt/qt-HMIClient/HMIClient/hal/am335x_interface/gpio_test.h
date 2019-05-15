#ifndef __GPIO__TEST__H__ 
#define __GPIO__TEST__H__

#ifdef __cplusplus
extern "C" {
#endif

enum gpio_value {GPIOOFF='0',GPIOON};

int create_gpio(char *path, char *name);
int create_all_gpios();
int gpio_is_output(char *direction_path);
//int gpio_set_value(int num,enum gpio_value value);

#if 0 
/*
 * @num; the serial num of gpio . Range from 0 to ARRAYSIZE(gpios[])-1. 
 * @return value : 0 --> success , 1 --> failed  
 */
int gpio_set(unsigned int num);
int gpio_reset(unsigned int num);

/*
 * return value : 0 means that gpio is reset 
 * 				  1 means that gpio is set 
 * 				 -1 means that get_value() failed 
 */
int get_gpio_status(unsigned int num);
#endif 


#ifdef __cplusplus
}
#endif

//void gpio_test();
#endif 
