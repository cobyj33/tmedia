#ifndef ASCII_VIDEO_THREADS
#define ASCII_VIDEO_THREADS
#include "media.h"
#include <mutex>

void sleep_for(long nanoseconds);
void sleep_for_sec(long secs);
void sleep_for_sec(double secs);
void sleep_for_ms(long ms);

/* void* video_playback_thread(MediaPlayer* player, std::mutex* alter_mutex); */
void* video_playback_thread(MediaPlayer* player, std::mutex& alter_mutex);
void* audio_playback_thread(MediaPlayer* player, std::mutex& alter_mutex);
void* data_loading_thread(MediaPlayer* player, std::mutex& alter_mutex);
/* void input_thread(void* args); */
void* render_loop(MediaPlayer* player, std::mutex& alter_mutex);
#endif
