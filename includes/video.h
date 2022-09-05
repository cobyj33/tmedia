#ifndef ASCII_VIDEO_PROGRAM
#define ASCII_VIDEO_PROGRAM

#define GIGABYTE_TO_BYTES 1000000000
#define VIDEO_FIFO_MAX_SIZE GIGABYTE_TO_BYTES / FRAME_DATA_SIZE
#define SECONDS_TO_NANOSECONDS 1000000000
#define SECONDS_TO_MILLISECONDS 1000
#define SYNC_THRESHOLD_MILLISECONDS 200000
#define PACKET_RESERVE_SIZE 100000
#define TIME_CHANGE_WAIT_MILLISECONDS 25
#define PLAYBACK_SPEED_CHANGE_WAIT_MILLISECONDS 250
#define PLAYBACK_SPEED_CHANGE_INTERVAL 0.25

#include <ascii_data.h>

extern "C" {
    #include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}



int videoProgram(const char* fileName);
int get_packet_stats(const char* fileName, int* videoPackets, int* audioPackets);

typedef struct Playback {
    int currentPacket;
    bool allPacketsRead = false;
    bool playing = false;
    int64_t skippedPTS;
    double speed;
    double time;
    double audioTime;
    double lastAudioPlayTime;
} Playback;


typedef enum VideoStateDisplayMode {
    MOVIE, VIDEO_DEBUG, AUDIO_DEBUG, WAVE
} VideoStateDisplayMode;

typedef struct VideoDisplaySettings {
    bool debug;
    bool subtitles;
    VideoStateDisplayMode mode;
} VideoDisplaySettings;

Playback* playback_alloc();
void playback_free(Playback* playback);


typedef struct VideoFrame {
    pixel_data* pixelData;
    int64_t pts;
    int repeat_pict;
    int64_t duration;
} VideoFrame;

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


#endif
