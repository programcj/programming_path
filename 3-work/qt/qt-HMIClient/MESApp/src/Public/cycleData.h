/*
 * cycleData.h
 *
 *  Created on: 2015年3月31日
 *      Author: zhuyp
 */

#ifndef CYCLEDATA_H_
#define CYCLEDATA_H_
#include <QDateTime>
#include <QtGlobal>
class CycleData
{
public:
  CycleData ();
  virtual
  ~CycleData ();

  // QTime FillTime;
 //  QTime CycleTime;

  //填充周期(S)
   quint32 fillTimeLong;
   //成型周期(s)
   quint32 cycleTimeLong;

   //开始时间
   QDateTime  moldstartTime;

   //结束时间
   QDateTime  moldendTime;

  const quint32& getFillTime() const
  {
      return fillTimeLong;
  }

  void setFillTime(quint32 filltime)
  {
    fillTimeLong = filltime;
  }

  const quint32& getCycleTime() const
  {
      return cycleTimeLong;
  }

  void setCycleTime(quint32 cycletime)
  {
    cycleTimeLong = cycletime;
  }

  void setstartTime(QDateTime startime)
  {
    moldstartTime = startime;
  }

  QDateTime & getstartTime()
  {
    return moldstartTime;
  }

  void setmoldendTime(QDateTime startime)
  {
    moldendTime = startime;
  }

  QDateTime & getmoldendTime()
  {
    return moldendTime;
  }

};

#endif /* CYCLEDATA_H_ */
