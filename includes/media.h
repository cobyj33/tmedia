#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "audiostream.h"
#include "decode.h"
#include "boiler.h"
#include "icons.h"
#include "playheadlist.hpp"
#include "color.h"
#include "playback.h"
#include "pixeldata.h"
#include <streamdata.h>

#include "gui.h"

#include <cstdint>
#include <vector>
#include <stack>
#include <memory>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}




class MediaStream {
    private:
        StreamData& info;

    public:
        PlayheadList<AVPacket*> packets;
        // double time_base;
        // double average_frame_rate;
        // double start_time;
        // int stream_index;
        // enum AVMediaType media_type;

        MediaStream(StreamData& streamData);
        ~MediaStream();

        double get_time_base() const;
        double get_average_frame_rate_sec() const;
        double get_average_frame_time_sec() const;
        double get_start_time() const;
        int get_stream_index() const;
        enum AVMediaType get_media_type() const;
        AVCodecContext* get_codec_context() const;
};

class MediaData {
    public:
        AVFormatContext* format_context;
        std::vector<std::unique_ptr<MediaStream>> media_streams;
        std::unique_ptr<StreamDataGroup> stream_datas;
        int nb_streams;
        bool allPacketsRead;
        double duration;

        MediaData(const char* file_name);
        ~MediaData();

        MediaStream& get_media_stream(enum AVMediaType media_type);
        bool has_media_stream(enum AVMediaType media_type);
        void fetch_next(int requestedPacketCount);
};

class Sample {
    public:
        float* data;
        int channels;

        Sample();
        ~Sample();
};


class MediaCache {
    public:
        PixelData image;
        PixelData last_rendered_image;
        PlayheadList<PixelData> image_buffer;
        AudioStream audio_stream;
        std::string message;

        /**
         * @brief A stack of the current video symbol and the time it was added to the stack
         */
        std::stack<std::tuple<VideoIcon, double>> symbol_stack;

        MediaCache() {}
};


class MediaPlayer {
    public:
        MediaCache cache;
        std::unique_ptr<MediaData> media_data;
        Playback playback;
        const char* file_name;
        bool in_use;

        MediaPlayer(const char* file_name) : in_use(false), file_name(file_name) {
            this->media_data = std::make_unique<MediaData>(file_name);
        }

        void start(GUIState gui_state);
        double get_desync_time(double current_system_time);
        void resync(double current_system_time);
        void set_current_image(PixelData& data);
        PixelData& get_current_image();

        double get_duration();
        void jump_to_time(double target_time);

        bool has_video();
        bool has_audio();
        MediaStream& get_video_stream();
        MediaStream& get_audio_stream();
};

void move_packet_list_to_pts(PlayheadList<AVPacket*>& packets, int64_t targetPTS);
void move_packet_list_to_time_sec(PlayheadList<AVPacket*>& packets, double time);
void move_frame_list_to_pts(PlayheadList<AVFrame*>& frames, int64_t targetPTS);
void move_frame_list_to_time_sec(PlayheadList<AVFrame*>& frames, double time);

void clear_playhead_packet_list(PlayheadList<AVPacket*>& packets);
void clear_playhead_frame_list(PlayheadList<AVFrame*>& packets);

#endif
