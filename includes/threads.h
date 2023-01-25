#ifndef ASCII_VIDEO_THREADS
#define ASCII_VIDEO_THREADS
#include "media.h"

extern "C" {
#include <pthread.h>
}

void sleep_for(long nanoseconds);
void sleep_for_sec(long secs);
void fsleep_for_sec(double secs);
void sleep_for_ms(long ms);

typedef struct MediaThreadData {
    MediaPlayer* player;
    pthread_mutex_t* alterMutex;
} MediaThreadData;

/* void* video_playback_thread(MediaPlayer* player, pthread_mutex_t* alterMutex); */
void* video_playback_thread(MediaThreadData* thread_data);
void* audio_playback_thread(MediaThreadData* thread_data);
void* data_loading_thread(MediaThreadData* thread_data);
/* void input_thread(void* args); */
void render_loop(MediaPlayer* player, pthread_mutex_t* alterMutex);
#endif
