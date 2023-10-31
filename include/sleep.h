#ifndef TMEDIA_THREADS_H
#define TMEDIA_THREADS_H

/**
 * @brief Sleep the current thread for the given number of nanoseconds
 * @param nanoseconds The number of nanoseconds to make the current thread sleep
 */
void sleep_for(long nanoseconds);

/**
 * @brief Sleep the current thread for the given number of seconds
 * @param secs The number of seconds to make the current thread sleep
 */
void sleep_for_sec(double secs);

/**
 * @brief Sleep the current thread for the given number of milliseconds
 * @param ms The number of milliseconds to make the current thread sleep
 */
void sleep_for_ms(long ms);

/**
 * @brief Sleep the current thread quickly and wake up almost immediately
 */
void sleep_quick();

#endif
