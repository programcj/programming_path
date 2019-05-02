/*
 * buzzer.h
 *
 *  Created on: 2015年3月20日
 *      Author: cj
 */

#ifndef BUZZER_H_
#define BUZZER_H_

class Buzzer
{
public:
  Buzzer ();
  virtual
  ~Buzzer ();
  int bzfd;

  void startBuzzer();
};

#endif /* BUZZER_H_ */
