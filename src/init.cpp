#include <cstdio>
#include <threads.h>
#include <media.h>
#include <wtime.h>

#include <thread>
#include <mutex>
#include <except.h>

void start_media_player_from_filename(const char* fileName) {
    MediaPlayer* player = media_player_alloc(fileName);
    if (player != NULL) {
        start_media_player(player);
        media_player_free(player);
    }
}

void start_media_player(MediaPlayer* player) {
    if (player->inUse) {
        throw ascii::forbidden_action_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    const int INITIAL_PLAYER_PACKET_BUFFER_SIZE = 5000;
    player->inUse = true;
    player->timeline->playback->start(system_clock_sec());
    fetch_next(player->timeline->mediaData, INITIAL_PLAYER_PACKET_BUFFER_SIZE);

    std::mutex alter_mutex;

    std::thread video_thread(video_playback_thread, player, std::ref(alter_mutex));
    std::thread audio_thread(audio_playback_thread, player, std::ref(alter_mutex));
    std::thread data_thread(data_loading_thread, player, std::ref(alter_mutex));
    render_loop(player, std::ref(alter_mutex));

    video_thread.join();
    audio_thread.join();
    data_thread.join();

    player->timeline->playback->stop(system_clock_sec());
    player->inUse = false;
}
