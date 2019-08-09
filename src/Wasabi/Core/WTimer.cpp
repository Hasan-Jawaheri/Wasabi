#include "Core/WTimer.h"
#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#include <mmsystem.h>					//time functions
#pragma comment ( lib, "winmm.lib" )	//timeGetTime function
#elif (defined __linux__)
#include <stdlib.h>
#endif

auto g_program_start_time = std::chrono::high_resolution_clock::now();

W_TIMER_TYPE _GetCurrentTime() {
	//calculate elapsed time
	auto tNow = std::chrono::high_resolution_clock::now();
	float count = std::chrono::duration_cast<std::chrono::microseconds>(tNow - g_program_start_time).count() / 1000000.0f;;
	return (W_TIMER_TYPE)count;
}

void WInitializeTimers() {
	//randomize timers
	srand(_GetCurrentTime());
}
void WUnInitializeTimers() {
}

WTimer::WTimer(float fUnit, bool bManualElapsedTime) {
	m_unit = fUnit;
	m_manualElapsedTime = bManualElapsedTime;

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
W_TIMER_TYPE WTimer::GetElapsedTime(bool bRecord) const {
	if (m_manualElapsedTime && !bRecord)
		return m_lastRecordedElapsedTime;

	W_TIMER_TYPE totalElapsedTime = _GetCurrentTime() - m_startTime;
	if (m_totalPauseTime + GetPauseTime() > totalElapsedTime) //dont allow negative values
		return 0;

	W_TIMER_TYPE elapsedTime = (totalElapsedTime - (m_totalPauseTime + GetPauseTime())) * m_unit; //multiply to convert unit
	if (bRecord)
		m_lastRecordedElapsedTime = elapsedTime;
	return elapsedTime;
}
W_TIMER_TYPE WTimer::GetPauseTime() const {
	if (m_pauseStartTime < 0)
		return 0;

	return _GetCurrentTime() - m_pauseStartTime;
};