#pragma once
#include <mutex>
#include "media.h"

void video_playback_thread(MediaPlayer* player, std::mutex* alterMutex);
int audio_playback_thread(MediaPlayer* player, std::mutex* alterMutex);
void data_loading_thread(MediaPlayer* player, std::mutex* alterMutex);
/* void input_thread(MediaPlayer* player, std::mutex* alterMutex); */
void render_thread(MediaPlayer* player, std::mutex* alterMutex);

