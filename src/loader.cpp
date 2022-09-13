#include <thread>
#include <chrono>
#include <loader.h>
#include <media.h>
#include <macros.h>

void data_loading_thread(MediaPlayer* player, std::mutex* alterMutex) {
    std::unique_lock<std::mutex> alterLock{*alterMutex, std::defer_lock};
    bool shouldFetch;
    MediaData* media_data = player->timeline->mediaData;

    while (!player->timeline->mediaData->allPacketsRead) {
        alterLock.lock();
        shouldFetch = true;

        for (int i = 0; i < media_data->nb_streams; i++) {
            if (media_data->media_streams[i]->packets->get_length() > PACKET_RESERVE_SIZE) {
                shouldFetch = false;
                break;
            }
        }

        if (shouldFetch) {
            fetch_next(media_data, 20);
        }

        alterLock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}


void fetch_next(MediaData* media_data, int requestedPacketCount) {
    AVPacket* readingPacket = av_packet_alloc();
    int result;
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
                break;
            }
        }

        av_packet_unref(readingPacket);
    }

    media_data->allPacketsRead = true;
    av_packet_free(&readingPacket);
}
