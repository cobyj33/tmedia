#ifndef ASCII_VIDEO_PROGRAM
#define ASCII_VIDEO_PROGRAM

#define GIGABYTE_TO_BYTES 1000000000
#define VIDEO_FIFO_MAX_SIZE GIGABYTE_TO_BYTES / FRAME_DATA_SIZE
#define SECONDS_TO_NANOSECONDS 1000000000
#define SYNC_THRESHOLD_MILLISECONDS 10
#define FRAME_RESERVE_SIZE 20000

#include <ascii_data.h>

extern "C" {
    #include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

typedef enum VideoIcon {
    STOP_ICON, PLAY_ICON, PAUSE_ICON, FORWARD_ICON, BACKWARD_ICON, MUTE_ICON, NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON, MAXIMIZED_ICON, MINIMIZED_ICON
} VideoIcon;

int videoProgram(const char* fileName);
int get_packet_stats(const char* fileName, int* videoPackets, int* audioPackets);

typedef struct Playback {
    int64_t currentPTS;
    int currentPacket;
    bool allPacketsRead = false;
    bool playing = false;
    int64_t skippedPTS;
} Playback;

typedef struct VideoSymbol {
    int framesToDelete;
    int framesShown;
    pixel_data* pixelData;
} VideoSymbol;

typedef struct VideoFrame {
    pixel_data* pixelData;
    int64_t pts;
    int repeat_pict;
    VideoFrame* next;
    VideoFrame* last;
} VideoFrame;

/* typedef struct VideoFrameList { */
/*     VideoFrame* first; */
/*     VideoFrame* last; */
/*     VideoFrame* current; */
/*     int currentPosition; */
/*     int length; */
/* } VideoFrameList; */

typedef struct AudioFrame {
    uint8_t *data[8];
    AVChannelLayout ch_layout;
    AVSampleFormat sample_fmt;
    int64_t pts;
    int nb_samples;
} AudioFrame;

VideoFrame* video_frame_alloc(AVFrame* avFrame);
void video_frame_free(VideoFrame* videoFrame);

AudioFrame* audio_frame_alloc(AVFrame* avFrame);
void audio_frame_free(AudioFrame* audioFrame);


int testIconProgram();
bool init_icons();
bool free_icons();
pixel_data* get_video_icon(VideoIcon iconEnum);
VideoSymbol get_video_symbol(VideoIcon iconEnum);
VideoSymbol get_symbol_from_volume(double normalizedVolume);

#endif
