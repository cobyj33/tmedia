#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "audiostream.h"
#include "decode.h"
#include "boiler.h"
#include "icons.h"
#include "debug.h"
#include "playheadlist.hpp"
#include "color.h"
#include "playback.h"
#include <streamdata.h>


#include <cstdint>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

typedef struct MediaStream {
    StreamData* info;
    PlayheadList<AVPacket*>* packets;
    double timeBase;
    decoder_function decodePacket;
} MediaStream;





// typedef struct Playback {
// } Playback;

typedef struct MediaData {
    AVFormatContext* formatContext;
    MediaStream** media_streams;
    StreamDataGroup* stream_datas;
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

// typedef struct AudioStream {
//     uint8_t* stream;
//     float start_time;
//     size_t nb_samples;
//     size_t playhead;
//     size_t sample_capacity;
//     int nb_channels;
//     int sample_rate;
// } AudioStream;


typedef struct Sample {
    float* data;
    int channels;
} Sample;

typedef struct MediaDisplayCache {
    MediaDebugInfo* debug_info;
    PixelData* image;
    PixelData* last_rendered_image;
    PlayheadList<PixelData*>* image_buffer;

    AudioStream* audio_stream;

    VideoSymbolStack* symbol_stack;
} MediaDisplayCache;

typedef struct MediaPlayer {
    MediaTimeline* timeline;
    MediaDisplaySettings* displaySettings;
    MediaDisplayCache* displayCache;
    const char* fileName;
    bool inUse;
} MediaPlayer;

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

MediaStream* get_media_stream(MediaData* media_data, enum AVMediaType media_type);
bool has_media_stream(MediaData* media_data, enum AVMediaType media_type);

void move_packet_list_to_pts(PlayheadList<AVPacket*>* packets, int64_t targetPTS);
void move_frame_list_to_pts(PlayheadList<AVFrame*>* frames, int64_t targetPTS);


void fetch_next(MediaData* media_data, int requestedPacketCount);
#endif
