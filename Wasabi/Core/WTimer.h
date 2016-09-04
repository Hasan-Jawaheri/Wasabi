/** @file WTimer.h
 *  @brief Basic timer implementation
 *
 *  @author Hasan Al-Jawaheri (hbj)
 *  @bug No known bugs.
 */

#pragma once

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
	WTimer(float fUnit = W_TIMER_MILLISECONDS);
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
	 * @return The elapsed time
	 */
	W_TIMER_TYPE GetElapsedTime() const;

private:
	/** The time at which the timer started */
	__int64 m_startTime;
	/** The time at which the last pause started */
	__int64 m_pauseStartTime;
	/** The total amount of time spent paused */
	__int64 m_totalPauseTime;
	/** "seconds per count" */
	float m_SPC;
	/** conversion constant */
	float m_unit;

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
