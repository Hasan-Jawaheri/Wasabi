#include "WTimer.h"
#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#include <mmsystem.h>					//time functions
#pragma comment ( lib, "winmm.lib" )	//timeGetTime function
#elif (defined __linux__)
#include <stdlib.h>
#endif

W_TIMER_TYPE _GetCurrentTime() {
	//calculate elapsed time
	auto tNow = std::chrono::high_resolution_clock::now();
	auto time = tNow.time_since_epoch().count();
	return (W_TIMER_TYPE)time;
}

void WInitializeTimers() {
	//randomize timers
	srand(_GetCurrentTime());
}
void WUnInitializeTimers() {
}

WTimer::WTimer(float fUnit) {
	m_unit = fUnit;

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
	return totalElapsedTime - (m_totalPauseTime + GetPauseTime()); //multiply to convert unit
}
W_TIMER_TYPE WTimer::GetPauseTime() const {
	if (m_pauseStartTime)
		return 0;

	return _GetCurrentTime() - m_pauseStartTime;
};