#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "audiobuffer.h"
#include "decode.h"
#include "boiler.h"
#include "icons.h"
#include "color.h"
#include "mediaclock.h"
#include "pixeldata.h"
#include "mediadecoder.h"

#include "audioresampler.h"

#include "gui.h"

#include <cstdint>
#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

class MediaPlayer {
  private:
    /**
     * @brief This loop is responsible for generating and updating the current video frame in the MediaPlayer's cache as the system clock runs.
     * @note This loop should be run in a separate thread
     * @note This loop should not output anything toward the stdout stream in any way
     * @param player The MediaPlayer to generate and update video frames for
     */
    void video_playback_thread();

    /**
     * @brief This loop is responsible for managing audio playback and synchronizing audio playback with the current MediaPlayer's timer
     * @note This loop should be run in a separate thread
     * @note This loop should not output anything toward the stdout stream in any way
     * @param player The MediaPlayer to generate and update video frames for
     */
    void audio_playback_thread();

    /**
     * @brief Some responsibilites of this thread are to process terminal input, render frames, and detect when the MediaPlayer has finished and send a notification in some way for other threads to end.
     * @note This loop should be run in the **main** thread, as output streams toward stdout with ncurses and iostream are not thread-safe. 
     * @param player The MediaPlayer to render toward the terminal
     */
    void render_loop();

    /**
     * @brief The currently loaded video frame from the given media player
     */
    PixelData frame;

    std::unique_ptr<MediaDecoder> media_decoder;

    std::unique_ptr<AudioResampler> audio_resampler;

    std::string error;

  public:
  
    /**
     * @brief The buffer of audio bytes loaded by the media player
     */
    std::unique_ptr<AudioBuffer> audio_buffer;


    std::mutex alter_mutex;
    std::mutex buffer_read_mutex;

    MediaGUI media_gui;

    MediaClock clock;
    const char* file_name;
    std::atomic<bool> in_use;
    std::atomic<bool> is_looped;
    std::atomic<bool> muted;
    std::atomic<double> volume;

    MediaType media_type;

    MediaPlayer(const char* file_name, MediaGUI starting_media_gui);

    void start(double start_time);

    double get_desync_time(double current_system_time) const;

    /**
     * @brief Sets the currently displayed frame of the currently playing media according to the PixelData
     * @return void 
     */
    void set_current_frame(PixelData& data);
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
     * @note The caller is responsible for making sure the time to jump to is in the bounds of the video's playtime. 
     * The video's duration could be found with the MediaPlayer::get_duration() function
     * 
     * @param target_time The target time to jump the playback to (must be reachable)
     * @param current_system_time The current system time
     * @throws If the target time is not in the boudns of the video's playtime
     */
    int jump_to_time(double target_time, double current_system_time);


    int load_next_audio();

    // StreamDecoder& get_stream_decoder(enum AVMediaType media_type) const;
    bool has_media_stream(enum AVMediaType media_type) const;

    bool has_error();
    std::string get_error();

};



#endif
