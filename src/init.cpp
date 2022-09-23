#include <chrono>
#include <threads.h>
#include <media.h>
#include <thread>
#include <mutex>

bool start_media_player_from_filename(const char* fileName) {
    MediaPlayer* player = media_player_alloc(fileName);
    if (player != nullptr) {
        start_media_player(player);
        media_player_free(player);
        return true;
    }
    return false;
}

void start_media_player(MediaPlayer* player) {
    if (player->inUse) {
        std::cerr << "CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE" << std::endl;
        return;
    }

    player->inUse = true;
    player->timeline->playback->playing = true;
    player->timeline->playback->clock->start_time = std::chrono::steady_clock::now();
    std::mutex alterMutex;

    std::thread video(video_playback_thread, player, &alterMutex);
    std::thread audio(audio_playback_thread, player, &alterMutex);
    std::thread bufferLoader(data_loading_thread, player, &alterMutex);
    render_thread(player, &alterMutex);
    
    video.join();
    audio.join();
    bufferLoader.join();

    player->timeline->playback->playing = false;
    player->inUse = false;
}
