/*
 * systeminfo.h
 *
 *  Created on: 2015年4月14日
 *      Author: zhuyp
 */

#ifndef SYSTEMINFO_H_
#define SYSTEMINFO_H_


namespace systemInfo
{
  //cpu 占用率
  float calc_cpu_using();
  //内存使用率
  bool calc_ram_using(long int &memtotal, long int &memfree);
  //flash 使用率
  // return true 返回容量和剩余空间有效
  bool calc_flash_using(long int &memtotal, long int &memfree);
  //SD 是否连接
  // 返回值 true SD连接 total 表示SD卡容量， free表示剩余空间
  // 返回 false 表示 SD没有连接
  bool calc_sd_using(long int &total, long int &free);
  //wifi 信号强度
  //FF = 读取不成功
  //0 = No Signal
  //1 = Very Low
  //2 = Low
  //3 = Good
  //4 = Very Good
  quint8 get_wifi_signal()

}


#endif /* SYSTEMINFO_H_ */
