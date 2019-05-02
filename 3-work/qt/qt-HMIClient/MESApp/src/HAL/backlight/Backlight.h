/*
 * Backlight.h
 *
 *  Created on: 2015年6月4日
 *      Author: cj
 */

#ifndef BACKLIGHT_H_
#define BACKLIGHT_H_

class Backlight
{
public:
	Backlight();
	virtual ~Backlight();
	static void closeScreen();
	static void lightScren();

};

#endif /* BACKLIGHT_H_ */
