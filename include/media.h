#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "audiobuffer.h"
#include "decode.h"
#include "boiler.h"
#include "icons.h"
#include "color.h"
#include "playback.h"
#include "pixeldata.h"
#include "streamdata.h"

#include "gui.h"

#include <cstdint>
#include <vector>
#include <stack>
#include <memory>
#include <string>
#include <deque>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}



class MediaPlayer {
    public:
        /**
         * @brief The currently loaded video frame from the given media player
         */
        PixelData frame;

        /**
         * @brief The buffer of audio bytes loaded by the media player
         */
        AudioBuffer audio_buffer;

        AVFormatContext* format_context;
        std::vector<std::unique_ptr<StreamData>> media_streams;

        MediaGUI media_gui;

        Playback playback;
        const char* file_name;
        bool in_use;
        bool is_looped;

        MediaPlayer(const char* file_name);
        MediaPlayer(const char* file_name, MediaGUI starting_media_gui);

        void start(double start_time);
        double get_desync_time(double current_system_time) const;

        /**
         * @brief Sets the currently displayed frame of the currently playing media according to the PixelData
         * @return void 
         */
        void set_current_frame(PixelData& data);

        /**
         * @brief Sets the currently displayed frame of the currently playing media according to the AVFrame.
         * @return void 
         */
        void set_current_frame(AVFrame* frame);

        /**
         * @brief Returns the currently displayed frame of the currently playing media
         * @return double 
         */
        PixelData& get_current_frame();

        /**
         * @brief Returns the duration in seconds of the currently playing media
         * @return double 
         */
        double get_duration() const;


        /**
         * @brief Returns the current timestamp of the video in seconds since the beginning of playback. This takes into account pausing, skipping, etc... and calculates the time according to the current system time given.
         * 
         * 
         * @return The current time of playback since 0:00 in seconds
         */
        double get_time(double current_system_time) const;

        /**
         * @brief Moves the MediaPlayer's playback to a certain time (including video and audio streams)
         * @note The caller is responsible for making sure the time to jump to is in the bounds of the video's playtime
         * 
         * @param target_time The target time to jump the playback to (must be reachable)
         * @param current_system_time The current system time
         * @throws 
         */
        void jump_to_time(double target_time, double current_system_time);

        /**
         * @brief Dictates whether this media player has an available video stream
         */
        bool has_video() const;

        /**
         * @brief Dictates whether this media player has an available audio stream
         */
        bool has_audio() const;

        /**
         * @brief Dictates whether this media player has only an available video media stream and no other available types of media streams
         */
        bool only_video() const;

        /**
         * @brief Dictates whether this media player has only an available audio media stream and no other available types of media streams
         */
        bool only_audio() const;

        std::vector<AVFrame*> next_video_frames(); 
        std::vector<AVFrame*> next_audio_frames();

        int load_next_audio_frames(int frames);

        StreamData& get_video_stream_data() const;
        StreamData& get_audio_stream_data() const;

        int get_nb_media_streams() const;
        StreamData& get_media_stream(enum AVMediaType media_type) const;
        bool has_media_stream(enum AVMediaType media_type) const;
        int fetch_next(int requestedPacketCount);

        ~MediaPlayer();
};


#endif
