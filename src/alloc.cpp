#include "boiler.h"
#include "decode.h"
#include <curses.h>
#include <media.h>
#include <info.h>
#include "doublelinkedlist.hpp"
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaPlayer* media_player_alloc(const char* fileName) {
    MediaPlayer* mediaPlayer = new MediaPlayer();
    mediaPlayer->displaySettings = media_display_settings_alloc();
    mediaPlayer->displayCache = media_display_cache_alloc();
    mediaPlayer->timeline = media_timeline_alloc(fileName);
    if (mediaPlayer->timeline == nullptr) {
        std::cerr << "Could not allocate media player because of error while allocating media timeline" << std::endl;
        delete mediaPlayer;
        return nullptr;
    }
    mediaPlayer->inUse = false;
    mediaPlayer->fileName = fileName;

    return mediaPlayer;
}

MediaDisplayCache* media_display_cache_alloc() {
    MediaDisplayCache* cache = (MediaDisplayCache*)malloc(sizeof(MediaDisplayCache));
    cache->debug_info = (MediaDebugInfo*)malloc(sizeof(MediaDebugInfo));
    cache->symbol_stack = video_symbol_stack_alloc();
    cache->image = nullptr;
    cache->last_rendered_image = nullptr;
    return cache;
}

VideoSymbolStack* video_symbol_stack_alloc() {
    VideoSymbolStack* stack = (VideoSymbolStack*)malloc(sizeof(VideoSymbolStack));
    stack->top = -1;
    return stack;
} 

MediaDisplaySettings* media_display_settings_alloc() {
    MediaDisplaySettings* settings = (MediaDisplaySettings*)malloc(sizeof(MediaDisplaySettings));
    settings->show_debug = false;
    settings->subtitles = false;
    settings->mode = VIDEO;
    settings->can_use_colors = has_colors();
    settings->can_change_colors = can_change_color();
    settings->use_colors = false;
    return settings;
}

MediaTimeline* media_timeline_alloc(const char* fileName) {
    MediaTimeline* timeline = (MediaTimeline*)malloc(sizeof(MediaTimeline));
    timeline->playback = playback_alloc();
    timeline->mediaData = media_data_alloc(fileName);
    if (timeline->mediaData == nullptr) {
        std::cerr << "Could not allocate media timeline of " << fileName << " because of error while fetching media data" << std::endl;
        playback_free(timeline->playback);
        free(timeline);
        return nullptr;
    }


    return timeline;
}

MediaData* media_data_alloc(const char* fileName) {
    int result;
    MediaData* mediaData = (MediaData*)malloc(sizeof(MediaData)); 
    mediaData->formatContext = open_format_context(fileName, &result);
    if (mediaData->formatContext == nullptr || result < 0) {
        std::cerr << "Could not allocate media data of " << fileName << " because of error while fetching file format data" << std::endl;
        free(mediaData);
        return nullptr;
    }

    //TODO: Add subtitle stream
    AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    int out_stream_count;
    StreamData** streamData  = alloc_stream_datas(mediaData->formatContext, mediaTypes, 2, &out_stream_count);
    if (streamData == nullptr) {
        std::cerr << "Could not fetch allocate media data of " << fileName << " because of error while reading stream data of file" << std::endl;
        avformat_free_context(mediaData->formatContext);
        free(mediaData);
        return nullptr;
    }

    mediaData->allPacketsRead = false;
    mediaData->currentPacket = 0;
    mediaData->totalPackets = get_num_packets(fileName);
    mediaData->nb_streams = out_stream_count;
    mediaData->media_streams = (MediaStream**)malloc(sizeof(MediaStream*) * out_stream_count);
    for (int i = 0; i < out_stream_count; i++) {
        mediaData->media_streams[i] = media_stream_alloc(streamData[i]);
    }

    mediaData->duration = (double)mediaData->formatContext->duration / AV_TIME_BASE;

    return mediaData;
}

MediaStream* media_stream_alloc(StreamData* streamData) {
    MediaStream* mediaStream = (MediaStream*)malloc(sizeof(MediaStream));
    mediaStream->info = streamData;
    mediaStream->packets = new DoubleLinkedList<AVPacket*>();
    mediaStream->streamTime = 0.0;
    mediaStream->timeBase = av_q2d(streamData->stream->time_base);
    mediaStream->decodePacket = get_stream_decoder(streamData->mediaType); 
    //TODO: STREAM DECODER FOR SUBTITLE DATA
    return mediaStream;
}

void media_player_free(MediaPlayer* player) {
    media_display_settings_free(player->displaySettings);
    media_timeline_free(player->timeline);
    media_display_cache_free(player->displayCache);
    delete player;
    player = nullptr;
}

void video_symbol_stack_free(VideoSymbolStack *stack) {
    free(stack);
    stack = nullptr;
}

void media_display_cache_free(MediaDisplayCache* cache) {
    free(cache->debug_info);
    free(cache->image);
    free(cache);
    cache = nullptr;
}

void media_display_settings_free(MediaDisplaySettings *settings) {
    free(settings);
    settings = nullptr;
}

void media_timeline_free(MediaTimeline *timeline) {
    playback_free(timeline->playback);
    media_data_free(timeline->mediaData);
    free(timeline);
    timeline = nullptr;
}

void media_data_free(MediaData* mediaData) {
    for (int i = 0; i < mediaData->nb_streams; i++) {
        media_stream_free(mediaData->media_streams[i]);
    }
    avformat_close_input(&(mediaData->formatContext));
    avformat_free_context(mediaData->formatContext);
    free(mediaData);
    mediaData = nullptr;
}

void media_stream_free(MediaStream* mediaStream) {
    delete mediaStream->packets;
    stream_data_free(mediaStream->info);
    free(mediaStream);
    mediaStream = nullptr;
}

Playback* playback_alloc() {
    Playback* playback = (Playback*)malloc(sizeof(Playback));
    playback->time = 0.0;
    playback->speed = 1.0;
    playback->volume = 1.0;
    playback->playing = false;
    playback->clock = media_clock_alloc();
    return playback; 
}

void playback_free(Playback* playback) {
    free(playback->clock);
    free(playback);
    playback = nullptr;
}

MediaClock* media_clock_alloc() {
    MediaClock* clock = (MediaClock*)malloc(sizeof(MediaClock));
    clock->realTimeElapsed = std::chrono::nanoseconds(0);
    clock->realTimeSkipped = std::chrono::nanoseconds(0);
    clock->realTimePaused = std::chrono::nanoseconds(0);
    return clock;
}
