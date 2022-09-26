#include "boiler.h"
#include "debug.h"
#include "decode.h"
#include "selectionlist.h"
#include <stdio.h>
#include <media.h>
#include <info.h>
#include <curses.h>

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

MediaPlayer* media_player_alloc(const char* fileName) {
    MediaPlayer* mediaPlayer = malloc(sizeof(MediaPlayer));
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

    settings->show_debug = false;
    settings->subtitles = false;
    settings->mode = DISPLAY_MODE_VIDEO;
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

    timeline->playback = playback_alloc();
    if (timeline->playback == NULL) {
        fprintf(stderr, "%s %s %s\n", "Could not allocate media timeline of", fileName, "because of error while allocating media playback");
        free(timeline);
        return NULL;
    }

    timeline->mediaData = media_data_alloc(fileName);
    if (timeline->mediaData == NULL) {
        fprintf(stderr, "%s %s %s\n", "Could not allocate media timeline of", fileName, "because of error while fetching media data");
        playback_free(timeline->playback);
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

    //TODO: Add subtitle stream
    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    int out_stream_count;
    StreamData** streamData  = alloc_stream_datas(mediaData->formatContext, mediaTypes, 2, &out_stream_count);
    if (streamData == NULL) {
        fprintf(stderr,"%s %s %s", "Could not fetch allocate media data of", fileName, "because of error while reading stream data of file");
        avformat_free_context(mediaData->formatContext);
        free(mediaData);
        return NULL;
    }

    mediaData->allPacketsRead = false;
    mediaData->currentPacket = 0;
    mediaData->totalPackets = get_num_packets(fileName);
    mediaData->nb_streams = out_stream_count;
    mediaData->media_streams = (MediaStream**)malloc(sizeof(MediaStream*) * out_stream_count);

    if (mediaData->media_streams == NULL) {
        fprintf(stderr,"%s %s %s", "Could not fetch allocate media data of", fileName, "because of error while reading stream data of file");
        avformat_free_context(mediaData->formatContext);
        stream_datas_free(streamData, out_stream_count);
        free(mediaData);
        return NULL;
    }

    for (int i = 0; i < out_stream_count; i++) {
        mediaData->media_streams[i] = media_stream_alloc(streamData[i]);
    }

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
    mediaStream->packets = selection_list_alloc();
    if (mediaStream->packets == NULL) {
        free(mediaStream);
        return NULL;
    }

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
    free(stack);
    stack = NULL;
}

void media_display_cache_free(MediaDisplayCache* cache) {
    free(cache->debug_info);
    free(cache->image);
    free(cache);
    cache = NULL;
}

void media_display_settings_free(MediaDisplaySettings *settings) {
    free(settings);
    free(settings->best_palette);
    settings = NULL;
}

void media_timeline_free(MediaTimeline *timeline) {
    playback_free(timeline->playback);
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
    free(mediaData);
    mediaData = NULL;
}

void media_stream_free(MediaStream* mediaStream) {
    selection_list_free(mediaStream->packets);
    stream_data_free(mediaStream->info);
    free(mediaStream);
    mediaStream = NULL;
}

Playback* playback_alloc() {
    Playback* playback = (Playback*)malloc(sizeof(Playback));
    playback->start_time = 0.0;
    playback->paused_time = 0.0;
    playback->skipped_time = 0.0;
    playback->speed = 1.0;
    playback->volume = 1.0;
    playback->playing = 0;
    return playback; 
}

void playback_free(Playback* playback) {
    free(playback);
    playback = NULL;
}
