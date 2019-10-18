/** @file WTimer.hpp
 *  @brief Basic timer implementation
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

#include <chrono>

#define W_TIMER_TYPE			float
#define W_TIMER_SECONDS			1
#define W_TIMER_MINUTES			(1.0f/60.0f)
#define W_TIMER_HOURS			((1.0f/60.0f)/60.0f)
#define W_TIMER_MILLISECONDS	1000
#define W_TIMER_MICROSECONDS	1000000

/**
 * @ingroup engineclass
 *
 * A timer can be used to to calculate elapsed times and can be controlled by
 * starting, pausing and stopping it.
 */
class WTimer {
public:
	WTimer(float fUnit = W_TIMER_MILLISECONDS, bool bManualElapsedTime = false);
	~WTimer();

	/**
	 * Start the timer.
	 */
	void Start();

	/**
	 * Pause the timer.
	 */
	void Pause();

	/**
	 * Reset the timer. This will also stop it.
	 */
	void Reset();

	/**
	 * Get the time elapsed since this timer was and until it was paused.
	 * @param bRecord  If true, the returned elapsed time will be recorded for
	 *                 future GetElapsedTime() calls (for m_manualElapsedTime
	 *                 = true)
	 * @return The elapsed time
	 */
	W_TIMER_TYPE GetElapsedTime(bool bRecord = false) const;

private:
	/** The time at which the timer started */
	long long m_startTime;
	/** The time at which the last pause started */
	long long m_pauseStartTime;
	/** The total amount of time spent paused */
	long long m_totalPauseTime;
	/** conversion constant */
	float m_unit;
	/** If set to true, GetElapsedTime() will return  */
	bool m_manualElapsedTime;
	/** Last time recorded by GetElapsedTime(true) */
	mutable W_TIMER_TYPE m_lastRecordedElapsedTime;

	/**
	 * Retrieves the total time spent paused
	 * @return Total time spent paused
	 */
	W_TIMER_TYPE GetPauseTime() const;
};

/**
 * Initializes the timers library. This is called by the engine.
 */
void WInitializeTimers();

/**
 * Un-initializes the timers library. This is called by the engine.
 */
void WUnInitializeTimers();
