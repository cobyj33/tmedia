#ifndef  TMEDIA_SLEEP_H
#define  TMEDIA_SLEEP_H

/**
 * Simple routines for 
*/

/**
 * @brief Sleep the current thread for the given number of nanoseconds
 * @param ns The number of nanoseconds to make the current thread sleep
 */
void sleep_for_ns(long ns);

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


#endif
