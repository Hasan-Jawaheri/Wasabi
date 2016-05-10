/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine timers helper library.
*********************************************************************/

#pragma once

#include "Common.h"

#define W_TIMER_TYPE			float
#define W_TIMER_SECONDS			1
#define W_TIMER_MINUTES			(1.0f/60.0f)
#define W_TIMER_HOURS			((1.0f/60.0f)/60.0f)
#define W_TIMER_MILLISECONDS	1000
#define W_TIMER_MICROSECONDS	1000000

/*********************************************************************
*********************************hxTimer*****************************
Timers class
*********************************************************************/
class WTimer {
public:
	WTimer(float fUnit = W_TIMER_MILLISECONDS);
	~WTimer(void);

	void				Start(void);
	void				Pause(void);
	void				Reset(void);

	W_TIMER_TYPE	GetElapsedTime(void) const;

private:
	__int64 m_startTime;
	__int64 m_pauseStartTime;
	__int64 m_totalPauseTime;
	float m_SPC;
	float m_unit;
	W_TIMER_TYPE GetPauseTime(void) const;
};

void WInitializeTimers();
