#ifndef ASCII_VIDEO_MEDIA
#define ASCII_VIDEO_MEDIA

#include "decode.h"
#include "boiler.h"
#include "gui.h"
#include "mediaclock.h"
#include "pixeldata.h"
#include "mediadecoder.h"
#include "audiobuffer.h"
#include "audioresampler.h"

#include <memory> // std::unique_ptr
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
  public:
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

    MediaType media_type;
    std::unique_ptr<MediaDecoder> media_decoder;
    PixelData frame;
    std::string error;
    std::mutex alter_mutex;
    std::mutex buffer_read_mutex;
    MediaClock clock;
    std::string file_path;
    std::atomic<bool> in_use;

    std::unique_ptr<AudioResampler> audio_resampler;
    std::unique_ptr<AudioBuffer> audio_buffer;


    MediaPlayer(const std::string& file_path);

    // void start(double start_time);

    double get_desync_time(double current_system_time) const;

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

    bool has_media_stream(enum AVMediaType media_type) const;
};



#endif
