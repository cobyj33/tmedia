#include "loader.h"
#include <cstdio>
#include <threads.h>
#include <media.h>
#include <wtime.h>

extern "C" {
#include <pthread.h>
}

int start_media_player_from_filename(const char* fileName) {
    MediaPlayer* player = media_player_alloc(fileName);
    if (player != NULL) {
        start_media_player(player);
        media_player_free(player);
        return 1;
    }

    return 0;
}

int start_media_player(MediaPlayer* player) {
    if (player->inUse) {
        fprintf(stderr, "%s","CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
        return 0;
    }

    player->inUse = true;
    player->timeline->playback->start();
    fetch_next(player->timeline->mediaData, 5000);

    pthread_mutex_t alterMutex;
    pthread_mutex_init(&alterMutex, NULL);

    MediaThreadData data = { player, &alterMutex };
    pthread_t video_thread, audio_thread, buffer_thread;
    int success = 1;
    int error;

    error = pthread_create(&video_thread, NULL, video_playback_thread, (void*)&data);
    if (error) {
        fprintf(stderr, "%s\n" ,"Failed to create video thread");
        success = 0;
        player->inUse = false;
    }

    error = pthread_create(&audio_thread, NULL, audio_playback_thread, (void*)&data);
    if (error) {
        fprintf(stderr, "%s\n" ,"Failed to create audio thread");
        success = 0;
        player->inUse = false;
    }

    error = pthread_create(&buffer_thread, NULL, data_loading_thread, (void*)&data);
    if (error) {
        fprintf(stderr, "%s\n" ,"Failed to create loading thread");
        success = 0;
        player->inUse = false;
    }
    render_loop(player, &alterMutex);

    error = pthread_join(video_thread, NULL);
    if (error) {
        fprintf(stderr, "%s\n", "Failed to join video thread");
        success = 0;
        player->inUse = false;
    }

    error = pthread_join(audio_thread, NULL);
    if (error) {
        fprintf(stderr, "%s\n", "Failed to join audio thread");
        success = 0;
        player->inUse = false;
    }

    error = pthread_join(buffer_thread, NULL);
    if (error) {
        fprintf(stderr, "%s\n", "Failed to join loading thread");
        success = 0;
        player->inUse = false;
    }

    player->timeline->playback->stop();
    player->inUse = 0;
    pthread_mutex_destroy(&alterMutex);
    return success;
}
