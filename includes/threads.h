#ifndef ASCII_VIDEO_THREADS
#define ASCII_VIDEO_THREADS
#include "media.h"
#include <mutex>

#include "gui.h"


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

/**
 * @brief This loop is responsible for generating and updating the current video frame in the MediaPlayer's cache as the system clock runs.
 * 
 * @note This loop should be run in a separate thread
 * @note This loop should not output anything toward the stdout stream in any way
 * 
 * @param player The MediaPlayer to generate and update video frames for
 * @param alter_mutex The mutex used to synchronize the threads 
 */
void video_playback_thread(MediaPlayer* player, std::mutex& alter_mutex);

/**
 * @brief This loop is responsible for managing audio playback and synchronizing audio playback with the current MediaPlayer's timer
 * 
 * @note This loop should be run in a separate thread
 * @note This loop should not output anything toward the stdout stream in any way
 * 
 * @param player The MediaPlayer to generate and update video frames for
 * @param alter_mutex The mutex used to synchronize the threads 
 */
void audio_playback_thread(MediaPlayer* player, std::mutex& alter_mutex);

/**
 * @brief This loop is responsible for loading 
 * 
 * @note This loop should be run in a separate thread
 * @note This loop should not output anything toward the stdout stream in any way
 * 
 * @param player The MediaPlayer to generate and update video frames for
 * @param alter_mutex The mutex used to synchronize the threads 
 */
void data_loading_thread(MediaPlayer* player, std::mutex& alter_mutex);

/**
 * 
 * @brief Some responsibilites of this thread are to process terminal input, render frames, and detect when the MediaPlayer has finished and send a notification in some way for other threads to end.
 * 
 * @note This loop should be run in the **main** thread, as output streams toward stdout with ncurses and iostream are not thread-safe. 
 * 
 * @param player The MediaPlayer to render toward the terminal
 * @param alter_mutex The mutex used to synchronize the threads 
 * @param gui_state The state of the GUI, usually configured from the command line at program invokation, to use to render.
 */
void render_loop(MediaPlayer* player, std::mutex& alter_mutex, GUIState gui_state);
#endif
