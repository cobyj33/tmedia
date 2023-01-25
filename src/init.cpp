#include <cstdio>
#include <threads.h>
#include <media.h>
#include <wtime.h>

#include <thread>
#include <except.h>

extern "C" {
#include <pthread.h>
}

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

    pthread_mutex_t alterMutex;
    pthread_mutex_init(&alterMutex, NULL);

    std::thread video_thread = std::thread(video_playback_thread, player, &alterMutex);
    std::thread audio_thread = std::thread(audio_playback_thread, player, &alterMutex);
    std::thread data_thread = std::thread(data_loading_thread, player, &alterMutex);
    render_loop(player, &alterMutex);

    video_thread.join();
    audio_thread.join();
    data_thread.join();

    player->timeline->playback->stop(system_clock_sec());
    player->inUse = false;
    pthread_mutex_destroy(&alterMutex);
}
