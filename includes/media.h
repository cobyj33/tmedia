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
#include "pixeldata.h"
#include <streamdata.h>


#include <cstdint>
#include <vector>
#include <stack>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}




class MediaStream {
    public:
        StreamData& info;
        PlayheadList<AVPacket*> packets;
        double timeBase;
        double average_frame_rate;
        double start_time;
        int stream_index;
        enum AVMediaType media_type;

        MediaStream(StreamData& streamData);
        ~MediaStream();

        double get_average_frame_rate();
        double get_start_time();
        int get_stream_index();
        enum AVMediaType get_media_type();
};

class MediaData {
    public:
        AVFormatContext* formatContext;
        std::vector<std::unique_ptr<MediaStream>> media_streams;
        std::unique_ptr<StreamDataGroup> stream_datas;
        int nb_streams;
        bool allPacketsRead;
        int currentPacket;
        double duration;

        MediaData(const char* fileName);
        ~MediaData();

        MediaStream& get_media_stream(enum AVMediaType media_type);
        bool has_media_stream(enum AVMediaType media_type);
        void fetch_next(int requestedPacketCount);
};



class MediaTimeline {
    public:
        std::unique_ptr<MediaData> mediaData;
        Playback playback;
        MediaTimeline(const char* fileName);
};


class MediaDisplaySettings {
    public:
        bool use_colors;

        MediaDisplaySettings();
};

class Sample {
    public:
        float* data;
        int channels;

        Sample();
        ~Sample();
};


class MediaDisplayCache {
    public:
        // MediaDebugInfo debug_info;
        PixelData image;
        PixelData last_rendered_image;
        PlayheadList<PixelData> image_buffer;
        AudioStream audio_stream;

        /**
         * @brief A stack of the current video symbol and the time it was added to the stack
         */
        std::stack<std::tuple<VideoIcon, double>> symbol_stack;

        MediaDisplayCache() {}
};


class MediaPlayer {
    public:
        std::unique_ptr<MediaTimeline> timeline;
        MediaDisplaySettings displaySettings;
        MediaDisplayCache displayCache;
        const char* fileName;
        bool inUse;

        MediaPlayer(const char* fileName) : inUse(false), fileName(fileName) {
            this->timeline = std::make_unique<MediaTimeline>(fileName);
        }

        void start();
        double get_desync_time(double current_system_time);
        void resync(double current_system_time);
        void set_current_image(PixelData& data);
        PixelData& get_current_image();
};

void move_packet_list_to_pts(PlayheadList<AVPacket*>& packets, int64_t targetPTS);
void move_packet_list_to_time_sec(PlayheadList<AVPacket*>& packets, double time);
void move_frame_list_to_pts(PlayheadList<AVFrame*>& frames, int64_t targetPTS);
void move_frame_list_to_time_sec(PlayheadList<AVFrame*>& frames, double time);

#endif
