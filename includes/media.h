#pragma once
#include "decode.h"
#include <cstdint>
#include <mutex>
#include "doublelinkedlist.hpp"
#include <chrono>
#include "boiler.h"
#include "icons.h"
#include "debug.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

typedef struct MediaStream {
    StreamData* info;
    DoubleLinkedList<AVPacket*>* packets;
    double streamTime;
    double timeBase;
    decoder_function decodePacket;
} MediaStream;

typedef struct MediaClock {
    std::chrono::nanoseconds realTimeElapsed;
    std::chrono::nanoseconds realTimeSkipped;
    std::chrono::nanoseconds realTimePaused;
    std::chrono::steady_clock::time_point start_time;
} MediaClock;

typedef struct Playback {
    bool playing;
    double speed;
    double time;
    double volume;
    MediaClock* clock;
} Playback;

typedef struct MediaData {
    AVFormatContext* formatContext;
    MediaStream** media_streams;
    int nb_streams;
    bool allPacketsRead;
    int currentPacket;
    int totalPackets;
    double duration;
} MediaData;

typedef struct MediaTimeline {
    MediaData* mediaData;
    Playback* playback;
} MediaTimeline;

typedef enum MediaDisplayMode {
    VIDEO, AUDIO
} MediaDisplayMode;

const int nb_display_modes = 2;
const MediaDisplayMode display_modes[nb_display_modes] = { VIDEO, AUDIO };
MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode);

typedef struct MediaDisplaySettings {
    bool show_debug;
    bool subtitles;
    bool use_colors;
    bool can_use_colors;
    bool can_change_colors;
    MediaDisplayMode mode;
} MediaDisplaySettings; 

typedef struct MediaDisplayCache {
    MediaDebugInfo* debug_info;
    PixelData* image;
    VideoSymbolStack* symbol_stack;
} MediaDisplayCache;

typedef struct MediaPlayer {
    MediaTimeline* timeline;
    MediaDisplaySettings* displaySettings;
    MediaDisplayCache* displayCache;
    const char* fileName;
    bool inUse;
} MediaPlayer;

/* typedef struct ThreadPriveliges { */
/*     bool masterTimeThread; */
/* } ThreadPriveliges; */

MediaPlayer* media_player_alloc(const char* fileName);
void media_player_free(MediaPlayer* player);


void start_media_player(MediaPlayer* player);
bool start_media_player_from_filename(const char* fileName);

MediaDisplayCache* media_display_cache_alloc();
void media_display_cache_free(MediaDisplayCache* cache);

MediaDisplaySettings* media_display_settings_alloc();
void media_display_settings_free(MediaDisplaySettings* settings);

MediaTimeline* media_timeline_alloc(const char* fileName);
void media_timeline_free(MediaTimeline* timeline);

MediaData* media_data_alloc(const char* fileName);
void media_data_free(MediaData* mediaData);

MediaStream* media_stream_alloc(StreamData* streamData);
void media_stream_free(MediaStream* mediaStream);

Playback* playback_alloc();
void playback_free(Playback* playback);

MediaClock* media_clock_alloc();

MediaStream* get_media_stream(MediaData* media_data, AVMediaType media_type);
bool has_media_stream(MediaData* media_data, AVMediaType media_type);

void move_packet_list_to_pts(DoubleLinkedList<AVPacket*>* packets, int64_t targetPTS);


int get_media_player_dump_size(MediaPlayer* player);
void dump_media_player(MediaPlayer* player, char* buffer);

int get_media_timeline_dump_size(MediaTimeline* timeline);
void dump_media_timeline(MediaTimeline* timeline, char* buffer);

int get_media_data_dump_size(MediaData* data);
void dump_media_data(MediaData* data, char* buffer);

int get_media_stream_dump_size(MediaStream* stream);
void dump_media_stream(MediaStream* stream, char* buffer);

int get_stream_info_dump_size(StreamData* stream_data);
void dump_stream_info(StreamData* stream_data, char* buffer);

int get_playback_dump_size(Playback* playback);
void dump_playback(Playback* playback, char* buffer);

