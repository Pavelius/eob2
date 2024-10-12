#include "win.h"

unsigned long start_random_seed;

void waitcputime(unsigned v) {
	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = (long long)v * -100000;
	auto hTimer = CreateWaitableTimerW(0, 1, 0);
	if(!hTimer)
		return;
	if(!SetWaitableTimer(hTimer, &liDueTime, 0, 0, 0, 0))
		return;
	WaitForSingleObject(hTimer, INFINITE);
}

unsigned long getcputime() {
    return GetTickCount();
}

extern "C" int time(void* p);

unsigned int randomseed() {
	return time(0);
}