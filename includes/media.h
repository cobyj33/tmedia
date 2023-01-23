#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "decode.h"
#include "boiler.h"
#include "icons.h"
#include "debug.h"
#include "selectionlist.h"
#include "color.h"

#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

typedef struct MediaStream {
    StreamData* info;
    SelectionList* packets;
    double timeBase;
    decoder_function decodePacket;
} MediaStream;


typedef struct Playback {
    int playing;
    double speed;
    double volume;
    double start_time;
    double paused_time;
    double skipped_time;
} Playback;

typedef struct MediaData {
    AVFormatContext* formatContext;
    MediaStream** media_streams;
    int nb_streams;
    int allPacketsRead;
    int currentPacket;
    int totalPackets;
    double duration;
} MediaData;

typedef struct MediaTimeline {
    MediaData* mediaData;
    Playback* playback;
} MediaTimeline;

#define NUMBER_OF_DISPLAY_MODES 2
typedef enum MediaDisplayMode {
    DISPLAY_MODE_VIDEO, DISPLAY_MODE_AUDIO
} MediaDisplayMode;
MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode);

typedef struct MediaDisplaySettings {
    int subtitles;

    int use_colors;
    int can_use_colors;
    int can_change_colors;

    int train_palette;
    int palette_size;
    rgb* best_palette;
} MediaDisplaySettings; 

typedef struct AudioStream {
    uint8_t* stream;
    float start_time;
    size_t nb_samples;
    size_t playhead;
    size_t sample_capacity;
    int nb_channels;
    int sample_rate;
} AudioStream;

typedef struct Sample {
    float* data;
    int channels;
} Sample;

typedef struct MediaDisplayCache {
    MediaDebugInfo* debug_info;
    PixelData* image;
    PixelData* last_rendered_image;
    SelectionList* image_buffer;

    AudioStream* audio_stream;

    VideoSymbolStack* symbol_stack;
} MediaDisplayCache;

typedef struct MediaPlayer {
    MediaTimeline* timeline;
    MediaDisplaySettings* displaySettings;
    MediaDisplayCache* displayCache;
    const char* fileName;
    int inUse;
} MediaPlayer;

/* typedef struct ThreadPriveliges { */
/*     int masterTimeThread; */
/* } ThreadPriveliges; */

MediaPlayer* media_player_alloc(const char* fileName);
void media_player_free(MediaPlayer* player);

int start_media_player(MediaPlayer* player);
int start_media_player_from_filename(const char* fileName);

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
double get_playback_current_time(Playback* playback);

AudioStream* audio_stream_alloc();
void audio_stream_free(AudioStream* stream);
int audio_stream_init(AudioStream* stream, int nb_channels, int initial_size, int sample_rate);
int audio_stream_clear(AudioStream* stream, int cleared_capacity);
double audio_stream_time(AudioStream* stream);
double audio_stream_end_time(AudioStream* stream);
double audio_stream_set_time(AudioStream* stream, double time);

MediaStream* get_media_stream(MediaData* media_data, enum AVMediaType media_type);
int has_media_stream(MediaData* media_data, enum AVMediaType media_type);

void move_packet_list_to_pts(SelectionList* packets, int64_t targetPTS);
void move_frame_list_to_pts(SelectionList* frames, int64_t targetPTS);


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
#endif
