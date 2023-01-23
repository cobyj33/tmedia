
#include "boiler.h"
#include "debug.h"
#include "decode.h"
#include "icons.h"
#include <selectionlist.h>
#include <media.h>
#include <info.h>

#include <cstdint>
#include <cstdio>

extern "C" {
#include <curses.h>
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
    cache->image_buffer = selection_list_alloc();
    if (cache->image_buffer == NULL) {
       fprintf(stderr, "%s", "Could not allocate Media Cache Image Buffer"); 
        media_debug_info_free(cache->debug_info);
        video_symbol_stack_free(cache->symbol_stack);
        free(cache);
        return NULL;
    }

    cache->audio_stream = audio_stream_alloc();
    if (cache->audio_stream == NULL) {
        free(cache->image_buffer);
        media_debug_info_free(cache->debug_info);
        video_symbol_stack_free(cache->symbol_stack);
        free(cache);
        return NULL;
    }

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
    video_symbol_stack_clear(stack);
    free(stack);
    stack = NULL;
}

void media_display_cache_free(MediaDisplayCache* cache) {
    free(cache->debug_info);
    free(cache->image_buffer);
    video_symbol_stack_free(cache->symbol_stack);
    if (cache->image != NULL) {
        free(cache->image);
    }
    if (cache->last_rendered_image != NULL) {
        free(cache->last_rendered_image);
    }
    if (cache->audio_stream != NULL) {
        audio_stream_free(cache->audio_stream);
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

AudioStream* audio_stream_alloc() {
    AudioStream* audio_stream = (AudioStream*)malloc(sizeof(AudioStream));
    if (audio_stream == NULL) {
        return NULL;
    }
    audio_stream->stream = NULL;
    audio_stream->playhead = 0;
    audio_stream->nb_channels = 0;
    audio_stream->start_time = 0.0;
    audio_stream->sample_capacity = 0;
    audio_stream->nb_samples = 0;
    audio_stream->sample_rate = 0;
    return audio_stream;
}

int audio_stream_init(AudioStream* stream, int nb_channels, int initial_size, int sample_rate) {
    if (stream->stream != NULL) {
        free(stream->stream);
    }

    stream->stream = (uint8_t*)malloc(sizeof(uint8_t) * nb_channels * initial_size * 2);
    if (stream->stream == NULL) {
        return 0;
    }

    stream->sample_capacity = initial_size * 2;
    stream->nb_samples = 0;
    stream->nb_channels = nb_channels;
    stream->start_time = 0.0;
    stream->sample_rate = sample_rate;
    stream->playhead = 0;
    return 1;
}

int audio_stream_clear(AudioStream* stream, int cleared_capacity) {
    if (stream->stream != NULL) {
        free(stream->stream);
    }

    uint8_t* tmp =  (uint8_t*)malloc(sizeof(uint8_t) * stream->nb_channels * cleared_capacity * 2);
    if (tmp == NULL) {
        return 0;
    }
    stream->stream = tmp;
    stream->nb_samples = 0;
    stream->playhead = 0;
    stream->sample_capacity = cleared_capacity;
    stream->playhead = 0;
    return 1;
}

void audio_stream_free(AudioStream* stream) {
    if (stream->stream != NULL) {
        free(stream->stream);
    }
    free(stream);
}
