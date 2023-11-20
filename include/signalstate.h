#ifndef TMEDIA_SIGNAL_STATE_H
#define TMEDIA_SIGNAL_STATE_H

/**
 * A simple header to define extern flags for when
 * signals are received for safe exit of program.
*/

/** should only be read on the main thread */
extern bool INTERRUPT_RECEIVED;

#endif