#pragma once

#include <Windows.h>

class RTMPPush
{
	char urlin[1024];
	char urlout[1024];
	bool _loop;

	char stat[300];
public:
	
	RTMPPush(const char* urlin, const char* urlout);

	~RTMPPush();

	bool start();

	void stop();

	const char* getURLOut();
	const char* getStatStr();
};

