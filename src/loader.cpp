#include <media.h>
#include <threads.h>
#include <mutex>

extern "C" {
#include <curses.h>
#include <libavutil/avutil.h>
}

bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex);

void data_loading_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    while (data_loading_thread_loop(player, alter_mutex)) {
        sleep_for_ms(30);
    }
}

bool data_loading_thread_loop(MediaPlayer* player, std::mutex& mutex) {
    std::lock_guard<std::mutex> lock_guard(mutex);
    MediaData& media_data = *player->timeline->mediaData;
    if (media_data.allPacketsRead || !player->inUse) {
        return false;
    }

    const int PACKET_RESERVE_SIZE = 100000;

    bool shouldFetch = true;

    for (int i = 0; i < media_data.nb_streams; i++) {
        if (media_data.media_streams[i]->packets.get_length() > PACKET_RESERVE_SIZE) {
            shouldFetch = false;
            break;
        }
    }

    if (shouldFetch) {
        media_data.fetch_next(20);
    }

    return true;
}

void MediaData::fetch_next(int requestedPacketCount) {
    AVPacket* readingPacket = av_packet_alloc();
    int packetsRead = 0;

    while (av_read_frame(this->formatContext, readingPacket) == 0) {
        this->currentPacket++;
        if (packetsRead >= requestedPacketCount) {
            return;
        }

        for (int i = 0; i < this->nb_streams; i++) {
            if (this->media_streams[i]->get_stream_index() == readingPacket->stream_index) {
                AVPacket* savedPacket = av_packet_alloc();
                av_packet_ref(savedPacket, readingPacket);
                this->media_streams[i]->packets.push_back(savedPacket);
                break;
            }
        }

        av_packet_unref(readingPacket);
    }

    this->allPacketsRead = true;
    av_packet_free(&readingPacket);
}
