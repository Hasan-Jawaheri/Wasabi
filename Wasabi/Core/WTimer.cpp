#include "WTimer.h"
#include <Windows.h>
#include <time.h>
#include <mmsystem.h>					//time functions
#pragma comment ( lib, "winmm.lib" )	//timeGetTime function

void WInitializeTimers() {
	//randomize timers
	srand(timeGetTime());

	//begin accurate timing system
	timeBeginPeriod(1);
}
void WUnInitializeTimers() {
	//end accurate timing system
	timeEndPeriod(1);
}

W_TIMER_TYPE _GetCurrentTime() {
	//calculate elapsed time
	__int64 currtime = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&currtime);
	return (W_TIMER_TYPE)currtime;
};

WTimer::WTimer(float fUnit) {
	m_unit = fUnit;

	//initialize accurate timing system
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_SPC = 1.0f / (float)countsPerSec;

	Reset();
}
WTimer::~WTimer() {
}
void WTimer::Start() {
	m_totalPauseTime += GetPauseTime();
	m_pauseStartTime = -1;
}
void WTimer::Pause() {
	m_pauseStartTime = _GetCurrentTime();
}
void WTimer::Reset() {
	m_startTime = _GetCurrentTime();
	m_totalPauseTime = 0;
	Pause();
}
W_TIMER_TYPE WTimer::GetElapsedTime() const {
	W_TIMER_TYPE totalElapsedTime = _GetCurrentTime() - m_startTime;
	if (m_totalPauseTime + GetPauseTime() > totalElapsedTime) //dont allow negative values
		return 0;
	return ((totalElapsedTime - (m_totalPauseTime + GetPauseTime()))*m_SPC)*m_unit; //multiply to convert unit
}
W_TIMER_TYPE WTimer::GetPauseTime() const {
	if (m_pauseStartTime)
		return 0;

	return _GetCurrentTime() - m_pauseStartTime;
};