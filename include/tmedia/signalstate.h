#ifndef TMEDIA_SIGNAL_STATE_H
#define TMEDIA_SIGNAL_STATE_H

/**
 * A simple boolean flag to note whether a catchable interrupt has
 * been sent to the tmedia process, and tmedia should then try
 * to exit the program gracefully and immediately.
 *
 * This flag must only be read from the main thread.
*/
extern bool INTERRUPT_RECEIVED;

#endif
