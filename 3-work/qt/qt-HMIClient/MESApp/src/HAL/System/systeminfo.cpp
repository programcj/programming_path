/*
 * systeminfo.cpp
 *
 *  Created on: 2015年4月14日
 *      Author: zhuyp
 */
//#include <QFile>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define  __DEBUG__  0

float usage=0.0;
long int all1=0;
long int all2=0;
long int idle1=0;
long int idle2=0;
/*总的Cpu使用率计算


计算方法：
1、  采样两个足够短的时间间隔的Cpu快照，分别记作t1,t2，其中t1、t2的结构均为：
(user、nice、system、idle、iowait、irq、softirq、stealstolen、guest)的9元组;
2、  计算总的Cpu时间片totalCpuTime
a)         把第一次的所有cpu使用情况求和，得到s1;
b)         把第二次的所有cpu使用情况求和，得到s2;
c)         s2 - s1得到这个时间间隔内的所有时间片，即totalCpuTime = j2 - j1 ;
3、计算空闲时间idle
idle对应第四列的数据，用第二次的idle - 第一次的idle即可
idle=第二次的idle - 第一次的idle
4、计算cpu使用率
pcpu =100* (total-idle)/total
*/
//返回cpu使用率
float calc_cpu_using()
{
  FILE *fp;
  char buf[128];
  char cpu[5];
  long int user,nice,sys,idle,iowait,irq,softirq;

  fp = fopen("/proc/stat","r");

  if(fp == NULL)
  {
      perror("fopen:");
      exit (0);
  }
  fgets(buf,sizeof(buf),fp);

    #if __DEBUG__
      printf("buf=%s",buf);
    #endif
      sscanf(buf,"%s%d%d%d%d%d%d%d",cpu,&user,&nice,&sys,&idle,&iowait,&irq,&softirq);

    #if __DEBUG__
      printf("%s,%d,%d,%d,%d,%d,%d,%d\n",cpu,user,nice,sys,idle,iowait,irq,softirq);
    #endif

      all2 = user+nice+sys+idle+iowait+irq+softirq;   /*cpu总的时间*/
      idle2 = idle;  /*空闲时间*/
      usage = (float)(all2-all1-(idle2-idle1)) / (all2-all1)*100 ;

      fclose(fp);
#if __DEBUG__
      printf("all=%d\n",all2-all1);
      printf("ilde=%d\n",all2-all1-(idle2-idle1));
      printf("cpu use = %.2f\%\n",usage);
      printf("=======================\n");
#endif
      all1 =all2;
      idle1=idle2;
     return usage;
}

//返回RAM使用率
bool calc_ram_using(long int &memtotal, long int &memfree)
{
  QFile f("/proc/meminfo");
  if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    qDebug() << "Open failed." << endl;
    return false;
  }
  QTextStream txtInput(&f);
  QString lineStr1,lineStr2;
  lineStr1 = txtInput.readLine();
  QStringList aa =  lineStr1.trimmed().split(" ",QString::SkipEmptyParts);
  memtotal = aa[1].toInt();
  lineStr2 = txtInput.readLine();
  QStringList bb =  lineStr2.trimmed().split(" ",QString::SkipEmptyParts);
  memfree = bb[1].toInt();
  f.close();
  return true;
}

//返回FLASH使用率
bool calc_flash_using(long int &memtotal, long int &memfree)
{

  QProcess *proc = new QProcess();
  proc->start("./storageinfo.sh");
  if (!proc->waitForFinished())
    return false;
  QByteArray totalsize = proc->readLine().trimmed();
  QByteArray freesize = proc->readLine().trimmed();
  if(totalsize.size()==0)
  {
      return false;
  }
  bool ok;
  memtotal = totalsize.toInt(&ok,10);
  memfree = freesize.toInt(&ok,10);
  qDebug()<< "totalsize:"<<memtotal<<"freesize:"<<freesize;
  if(ok)
  {
      return true;
  }
  else
  {
     return false;
  }

}

//返回SD使用率
bool calc_sd_using(long int &total, long int &free)
{
   QProcess *proc = new QProcess();
   proc->start("./storageinfo.sh");
   if (!proc->waitForFinished())
     return false;
   QByteArray totalsize = proc->readLine().trimmed();
   QByteArray freesize = proc->readLine().trimmed();
   if(totalsize.size()==0)
   {
       return false;
   }
   bool ok;
   total = totalsize.toInt(&ok,10);
   free = freesize.toInt(&ok,10);
   qDebug()<< "totalsize:"<<memtotal<<"freesize:"<<freesize;
   if(ok)
   {
       return true;
   }
   else
   {
      return false;
   }

}

//wifi信号强度
//FF = 读取不成功
//0 = No Signal
//1 = Very Low
//2 = Low
//3 = Good
//4 = Very Good
quint8 get_wifi_signal()
{
   quint8 siglevel=0xff;
   QProcess *proc = new QProcess();
   proc->start("cat /proc/net/wireless |grep wlan0");
   if (!proc->waitForFinished())
      return false;
   QByteArray signal = proc->readAllStandardOutput();
   signal = signal.mid(signal.indexOf("wlan0"),-1);
   //qDebug()<< signal;
   QString str(signal);
   QStringList listtext = str.split(" ",QString::SkipEmptyParts);
   //qDebug()<<listtext[3];
    if(listtext[3].endsWith('.'))
    {
        signal = listtext[3].left(listtext[3].size() -1).toLatin1();
        bool ok;
        int level;
        level =signal.toInt(&ok,10);
        if(ok)
        {
            if(level >= 80)
              siglevel= 4;
            else if(level >=60)
              siglevel= 3;
            else if(level >= 40)
              siglevel = 2;
            else if(level >= 10)
              siglevel = 1;
            else
              siglevel = 0;
         }
    }

   return siglevel;
}
