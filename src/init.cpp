#include <cstdio>
#include <thread>
#include <mutex>

#include "threads.h"
#include "media.h"
#include "wtime.h"
#include "except.h"

#include "gui.h"

void MediaPlayer::start(GUIState gui_state) {
    if (this->inUse) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    const int INITIAL_PLAYER_PACKET_BUFFER_SIZE = 10000;
    this->inUse = true;
    this->timeline->playback.start(system_clock_sec());
    this->timeline->mediaData->fetch_next(INITIAL_PLAYER_PACKET_BUFFER_SIZE);


    std::mutex alter_mutex;
    std::thread video_thread(video_playback_thread, this, std::ref(alter_mutex));
    std::thread audio_thread(audio_playback_thread, this, std::ref(alter_mutex));
    std::thread data_thread(data_loading_thread, this, std::ref(alter_mutex));
    render_loop(this, std::ref(alter_mutex), gui_state);

    video_thread.join();
    audio_thread.join();
    data_thread.join();

    this->timeline->playback.stop(system_clock_sec());
    this->inUse = false;
}
