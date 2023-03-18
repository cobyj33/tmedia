#include <cstdio>
#include <thread>
#include <mutex>

#include "threads.h"
#include "media.h"
#include "wtime.h"
#include "except.h"

#include "gui.h"

void MediaPlayer::start(MediaGUI media_gui, double start_time) {
    if (this->in_use) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    this->in_use = true;
    this->playback.start(system_clock_sec());
    this->jump_to_time(start_time, system_clock_sec());


    std::mutex alter_mutex;
    std::thread video_thread(video_playback_thread, this, std::ref(alter_mutex));
    std::thread audio_thread(audio_playback_thread, this, std::ref(alter_mutex));
    // std::thread data_thread(data_loading_thread, this, std::ref(alter_mutex));
    render_loop(this, std::ref(alter_mutex), media_gui);

    video_thread.join();
    audio_thread.join();
    // data_thread.join();

    this->playback.stop(system_clock_sec());
    this->in_use = false;
}
