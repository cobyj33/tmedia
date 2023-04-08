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
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}



class MediaPlayer {
    private:
        void start_image();
        void start_video(double start_time);
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

        std::mutex alter_mutex;

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

/**
 * @brief This loop is responsible for generating and updating the current video frame in the MediaPlayer's cache as the system clock runs.
 * 
 * @note This loop should be run in a separate thread
 * @note This loop should not output anything toward the stdout stream in any way
 * 
 * @param player The MediaPlayer to generate and update video frames for
 * @param alter_mutex The mutex used to synchronize the threads 
 */
void video_playback_thread(MediaPlayer* player);

/**
 * @brief This loop is responsible for managing audio playback and synchronizing audio playback with the current MediaPlayer's timer
 * 
 * @note This loop should be run in a separate thread
 * @note This loop should not output anything toward the stdout stream in any way
 * 
 * @param player The MediaPlayer to generate and update video frames for
 * @param alter_mutex The mutex used to synchronize the threads 
 */
void audio_playback_thread(MediaPlayer* player);

/**
 * 
 * @brief Some responsibilites of this thread are to process terminal input, render frames, and detect when the MediaPlayer has finished and send a notification in some way for other threads to end.
 * 
 * @note This loop should be run in the **main** thread, as output streams toward stdout with ncurses and iostream are not thread-safe. 
 * 
 * @param player The MediaPlayer to render toward the terminal
 * @param alter_mutex The mutex used to synchronize the threads 
 * @param media_gui The state of the GUI, usually configured from the command line at program invokation, to use to render.
 */
void render_loop(MediaPlayer* player);


#endif
