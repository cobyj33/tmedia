#include <mutex>

#include "media.h"
#include "threads.h"

extern "C" {
#include <curses.h>
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
    MediaData& media_data = *player->timeline->mediaData;
    if (!player->inUse) {
        return false;
    }
    media_data.fetch_next(40);
    return true;
}

void MediaData::fetch_next(int requestedPacketCount) {
    AVPacket* readingPacket = av_packet_alloc();
    int packetsRead = 0;

    while (av_read_frame(this->formatContext, readingPacket) == 0) {
       
        for (int i = 0; i < this->nb_streams; i++) {
            if (this->media_streams[i]->get_stream_index() == readingPacket->stream_index) {
                AVPacket* savedPacket = av_packet_alloc();
                av_packet_ref(savedPacket, readingPacket);
                this->media_streams[i]->packets.push_back(savedPacket);
                packetsRead++;
                break;
            }
        }

        av_packet_unref(readingPacket);

         if (packetsRead >= requestedPacketCount) {
            av_packet_free(&readingPacket);
            return;
        }
    }

    av_packet_free(&readingPacket);
}
