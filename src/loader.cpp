#include <media.h>
#include <threads.h>
#include <mutex>

extern "C" {
#include <curses.h>
#include <libavutil/avutil.h>
}

bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex);


void* data_loading_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    while (data_loading_thread_loop(player, alter_mutex)) {
        sleep_for_ms(30);
    }

    return NULL;
}

bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex) {
    std::lock_guard<std::mutex> lock_guard(mutex);
    MediaData* media_data = player->timeline->mediaData;
    if (media_data->allPacketsRead || !player->inUse) {
        return false;
    }

    const int PACKET_RESERVE_SIZE = 100000;

    bool shouldFetch = true;

    for (int i = 0; i < media_data->nb_streams; i++) {
        if (media_data->media_streams[i]->packets->get_length() > PACKET_RESERVE_SIZE) {
            shouldFetch = false;
            break;
        }
    }

    if (shouldFetch) {
        fetch_next(media_data, 20);
    }

    return true;
}


void fetch_next(MediaData* media_data, int requestedPacketCount) {
    AVPacket* readingPacket = av_packet_alloc();
    int packetsRead = 0;

    while (av_read_frame(media_data->formatContext, readingPacket) == 0) {
        media_data->currentPacket++;
        if (packetsRead >= requestedPacketCount) {
            return;
        }

        for (int i = 0; i < media_data->nb_streams; i++) {
            if (media_data->media_streams[i]->info->stream->index == readingPacket->stream_index) {
                AVPacket* savedPacket = av_packet_alloc();
                av_packet_ref(savedPacket, readingPacket);
                media_data->media_streams[i]->packets->push_back(savedPacket);
                /* erase(); */
                /* printw("Packet List For %s: %d", av_get_media_type_string(media_data->media_streams[i]->info->mediaType), media_data->media_streams[i]->packets->get_length()); */
                /* refresh(); */
                break;
            }
        }

        av_packet_unref(readingPacket);
    }

    media_data->allPacketsRead = 1;
    av_packet_free(&readingPacket);
}
