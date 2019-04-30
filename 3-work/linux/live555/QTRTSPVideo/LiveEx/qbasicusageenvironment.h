#ifndef QBASICUSAGEENVIRONMENT_H
#define QBASICUSAGEENVIRONMENT_H

#ifndef _BASIC_USAGE_ENVIRONMENT0_HH
#include "BasicUsageEnvironment0.hh"
#endif
#include <QtCore>

class QBasicUsageEnvironment : public BasicUsageEnvironment0
{
public:
  static QBasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

  // redefined virtual functions:
  virtual int getErrno() const;

  virtual UsageEnvironment& operator<<(char const* str);
  virtual UsageEnvironment& operator<<(int i);
  virtual UsageEnvironment& operator<<(unsigned u);
  virtual UsageEnvironment& operator<<(double d);
  virtual UsageEnvironment& operator<<(void* p);

protected:
  QBasicUsageEnvironment(TaskScheduler& taskScheduler);
      // called only by "createNew()" (or subclass constructors)
  virtual ~QBasicUsageEnvironment();
};

#endif // QBASICUSAGEENVIRONMENT_H
