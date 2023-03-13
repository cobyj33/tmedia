#include <cstdio>
#include <thread>
#include <mutex>

#include "threads.h"
#include "media.h"
#include "wtime.h"
#include "except.h"

#include "gui.h"

void MediaPlayer::start(GUIState gui_state, double start_time) {
    if (this->in_use) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    const int INITIAL_PLAYER_PACKET_BUFFER_SIZE = 100000;
    this->in_use = true;
    this->playback.start(system_clock_sec());
    this->media_data->fetch_next(INITIAL_PLAYER_PACKET_BUFFER_SIZE);
    // this->jump_to_time(start_time, system_clock_sec());


    std::mutex alter_mutex;
    std::thread video_thread(video_playback_thread, this, std::ref(alter_mutex));
    std::thread audio_thread(audio_playback_thread, this, std::ref(alter_mutex));
    std::thread data_thread(data_loading_thread, this, std::ref(alter_mutex));
    render_loop(this, std::ref(alter_mutex), gui_state);

    video_thread.join();
    audio_thread.join();
    data_thread.join();

    this->playback.stop(system_clock_sec());
    this->in_use = false;
}
