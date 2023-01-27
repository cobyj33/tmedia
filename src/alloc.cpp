
#include "boiler.h"
#include "debug.h"
#include "decode.h"
#include "icons.h"
#include <playheadlist.hpp>
#include <media.h>
#include <info.h>
#include <streamdata.h>

#include <cstdint>
#include <cstdio>
#include <string>

extern "C" {
#include <ncurses.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaPlayer* media_player_alloc(const char* fileName) {
    MediaPlayer* mediaPlayer = (MediaPlayer*)malloc(sizeof(MediaPlayer));
    if (mediaPlayer == NULL) {
        fprintf(stderr, "%s", "Could not allocate media player");
        return NULL;
    }

    mediaPlayer->displaySettings = media_display_settings_alloc();
    if (mediaPlayer->displaySettings == NULL) {
        fprintf(stderr, "%s\n" ,"Could not allocate media player because of error while allocating media display settings");
        free(mediaPlayer);
        return NULL;
    }

    mediaPlayer->displayCache = media_display_cache_alloc();
    if (mediaPlayer->displayCache == NULL) {
        fprintf(stderr, "%s\n" ,"Could not allocate media player because of error while allocating media display cache");
        media_display_settings_free(mediaPlayer->displaySettings);
        free(mediaPlayer);
        return NULL;
    }

    mediaPlayer->timeline = media_timeline_alloc(fileName);
    if (mediaPlayer->timeline == NULL) {
        fprintf(stderr, "%s\n" ,"Could not allocate media player because of error while allocating media timeline");
        media_display_settings_free(mediaPlayer->displaySettings);
        media_display_cache_free(mediaPlayer->displayCache);
        free(mediaPlayer);
        return NULL;
    }

    mediaPlayer->inUse = false;
    mediaPlayer->fileName = fileName;
    return mediaPlayer;
}

MediaDisplayCache* media_display_cache_alloc() {
    MediaDisplayCache* cache = (MediaDisplayCache*)malloc(sizeof(MediaDisplayCache));
    if (cache == NULL) {
       fprintf(stderr, "%s", "Could not allocate Media Display Cache"); 
       return NULL;
    }

    cache->debug_info = media_debug_info_alloc();
    if (cache->debug_info == NULL) {
       fprintf(stderr, "%s", "Could not allocate Media Debug Info"); 
       free(cache);
       return NULL;
    }

    cache->symbol_stack = video_symbol_stack_alloc();
    if (cache->symbol_stack == NULL) {
       fprintf(stderr, "%s", "Could not allocate Media Debug Info"); 
       media_debug_info_free(cache->debug_info);
       free(cache);
       return NULL;
    }

    cache->image = NULL;
    cache->image_buffer = new PlayheadList<PixelData*>();
    cache->audio_stream = new AudioStream();
    cache->last_rendered_image = NULL;
    return cache;
}

MediaDebugInfo* media_debug_info_alloc() {
    MediaDebugInfo* debug_info = (MediaDebugInfo*)malloc(sizeof(MediaDebugInfo));
    if (debug_info == NULL) {
        fprintf(stderr, "%s", "Could not allocate Media Debug Info");
        return NULL;
    }
    debug_info->nb_messages = 0;
    return debug_info;
}

void media_debug_info_free(MediaDebugInfo* info) {
    clear_media_debug(info, "", "");
    free(info);
}

VideoSymbolStack* video_symbol_stack_alloc() {
    VideoSymbolStack* stack = (VideoSymbolStack*)malloc(sizeof(VideoSymbolStack));
    if (stack == NULL) {
        fprintf(stderr, "%s", "Could not allocate Video Symbol Stack");
        return NULL;
    }

    stack->top = -1;
    return stack;
} 

MediaDisplaySettings* media_display_settings_alloc() {
    MediaDisplaySettings* settings = (MediaDisplaySettings*)malloc(sizeof(MediaDisplaySettings));
    if (settings == NULL) {
        fprintf(stderr,  "%s", "Could not allocate Media Display Settings");
        return NULL;
    }

    settings->subtitles = false;
    settings->can_use_colors = has_colors() == TRUE ? 1 : 0;
    settings->can_change_colors = can_change_color() == TRUE ? 1 : 0;
    settings->use_colors = false;

    settings->train_palette = true;
    settings->best_palette = (rgb*)malloc(sizeof(rgb) * 16);
    if (settings->best_palette == NULL) {
        fprintf(stderr, "%s\n", "Could not allocate best color palette container for media player");
        free(settings);
        return NULL;
    }

    settings->palette_size = 16;

    return settings;
}

MediaTimeline* media_timeline_alloc(const char* fileName) {
    MediaTimeline* timeline = (MediaTimeline*)malloc(sizeof(MediaTimeline));
    if (timeline == NULL) {
        fprintf(stderr, "%s", "Could not allocate media timeline");
        return NULL;
    }

    timeline->playback = new Playback();

    timeline->mediaData = media_data_alloc(fileName);
    if (timeline->mediaData == NULL) {
        fprintf(stderr, "%s %s %s\n", "Could not allocate media timeline of", fileName, "because of error while fetching media data");
        delete timeline->playback;
        free(timeline);
        return NULL;
    }


    return timeline;
}

MediaData* media_data_alloc(const char* fileName) {
    int result;
    MediaData* mediaData = (MediaData*)malloc(sizeof(MediaData)); 
    if (mediaData == NULL) {
        fprintf(stderr, "%s %s\n", "Could not allocate media data of", fileName );
    }

    mediaData->formatContext = open_format_context(fileName, &result);
    if (mediaData->formatContext == NULL || result < 0) {
        fprintf(stderr, "%s %s %s\n", "Could not allocate media data of", fileName, "because of error while fetching file format data");
        free(mediaData);
        return NULL;
    }

    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    StreamDataGroup* stream_data_group  = new StreamDataGroup(mediaData->formatContext, mediaTypes, 2);

    mediaData->allPacketsRead = false;
    mediaData->currentPacket = 0;
    mediaData->totalPackets = get_num_packets(fileName);
    mediaData->nb_streams = stream_data_group->get_nb_streams();
    mediaData->media_streams = (MediaStream**)malloc(sizeof(MediaStream*) * mediaData->nb_streams);

    if (mediaData->media_streams == NULL) {
        fprintf(stderr,"%s %s %s", "Could not fetch allocate media data of", fileName, "because of error while reading stream data of file");
        avformat_free_context(mediaData->formatContext);
        free(mediaData);
        return NULL;
    }


    for (int i = 0; i < mediaData->nb_streams; i++) {
        mediaData->media_streams[i] = media_stream_alloc((*stream_data_group)[i]);
    }
    mediaData->stream_datas = stream_data_group;

    mediaData->duration = (double)mediaData->formatContext->duration / AV_TIME_BASE;

    return mediaData;
}

MediaStream* media_stream_alloc(StreamData* streamData) {
    MediaStream* mediaStream = (MediaStream*)malloc(sizeof(MediaStream));
    if (mediaStream == NULL) {
        fprintf(stderr, "%s", "Could not allocate Media Stream");
        return NULL;
    }

    mediaStream->info = streamData;
    mediaStream->packets = new PlayheadList<AVPacket*>();

    mediaStream->timeBase = av_q2d(streamData->stream->time_base);
    mediaStream->decodePacket = get_stream_decoder(streamData->mediaType); 
    //TODO: STREAM DECODER FOR SUBTITLE DATA
    return mediaStream;
}

void media_player_free(MediaPlayer* player) {
    media_display_settings_free(player->displaySettings);
    media_timeline_free(player->timeline);
    media_display_cache_free(player->displayCache);

    free(player);
    player = NULL;
}

void video_symbol_stack_free(VideoSymbolStack *stack) {
    video_symbol_stack_clear(stack);
    free(stack);
    stack = NULL;
}

void media_display_cache_free(MediaDisplayCache* cache) {
    free(cache->debug_info);
    delete cache->image_buffer;
    video_symbol_stack_free(cache->symbol_stack);
    if (cache->image != NULL) {
        free(cache->image);
    }
    if (cache->last_rendered_image != NULL) {
        free(cache->last_rendered_image);
    }
    if (cache->audio_stream != nullptr) {
        delete cache->audio_stream;
    }

    free(cache);
    cache = NULL;
}

void media_display_settings_free(MediaDisplaySettings *settings) {
    free(settings);
    free(settings->best_palette);
    settings = NULL;
}

void media_timeline_free(MediaTimeline *timeline) {
    delete timeline->playback;
    media_data_free(timeline->mediaData);
    free(timeline);
    timeline = NULL;
}

void media_data_free(MediaData* mediaData) {
    for (int i = 0; i < mediaData->nb_streams; i++) {
        media_stream_free(mediaData->media_streams[i]);
    }
    avformat_close_input(&(mediaData->formatContext));
    avformat_free_context(mediaData->formatContext);
    delete mediaData->stream_datas;
    free(mediaData);
    mediaData = NULL;
}

void media_stream_free(MediaStream* mediaStream) {
    delete mediaStream->packets;
    delete mediaStream->info;
    free(mediaStream);
    mediaStream = NULL;
}

