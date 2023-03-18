#include <mutex>

#include "media.h"
#include "threads.h"

extern "C" {
#include <libavutil/avutil.h>
}

bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex);

void data_loading_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    while (data_loading_thread_loop(player, alter_mutex)) {
        sleep_for_ms(60);
    }
}


bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex) {
    std::lock_guard<std::mutex> lock_guard(mutex);

    const int MAX_PACKET_LENGTH = 100;

    if (!player->in_use) {
        return false; // exit thread
    }

    int audio_packet_length = 0;
    int video_packet_length = 0;

    if (player->has_audio()) {
        StreamData& audio_stream = player->get_audio_stream();
        audio_packet_length = audio_stream.packets.get_length();
    }

    if (player->has_video()) {
        StreamData& video_stream = player->get_video_stream();
        video_packet_length = video_stream.packets.get_length();
    }

    if ((player->has_audio() && audio_packet_length < MAX_PACKET_LENGTH) || ( player->has_video() && video_packet_length < MAX_PACKET_LENGTH)) {
        player->media_data->fetch_next(40);
    }

    return true;
}

