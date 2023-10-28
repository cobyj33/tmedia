#ifndef ASCII_VIDEO_MEDIA_FETCHER_H
#define ASCII_VIDEO_MEDIA_FETCHER_H

#include "mediaclock.h"
#include "pixeldata.h"
#include "mediadecoder.h"
#include "audiobuffer.h"
#include "audioresampler.h"
#include "scale.h"

#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <optional>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

/**
 * 
 * Notes for use:
 * 
 * If both must be locked, ALWAYS lock the alter_mutex before the audio_buffer_mutex
*/
class MediaFetcher {
  private:
    std::thread video_thread;
    std::thread audio_thread;
    std::thread duration_checking_thread;
    void video_fetching_thread_func();
    void audio_dispatch_thread_func();
    void duration_checking_thread_func();

    void audio_sync_thread_func();
    void audio_fetching_thread_func();
    void buffer_size_management_thread_func();

  public:

    MediaType media_type;
    std::unique_ptr<MediaDecoder> media_decoder;
    PixelData frame;

    std::string error;
    std::mutex alter_mutex;
    std::mutex audio_buffer_mutex;
    std::condition_variable audio_buffer_cond;

    MediaClock clock;
    std::string file_path;
    std::atomic<bool> in_use;

    std::unique_ptr<AudioBuffer> audio_buffer;

    std::optional<VideoDimensions> requested_frame_dims;

    MediaFetcher(const std::string& file_path);

    void begin(); // Only to be called by owning thread
    void join(); // Only to be called by owning thread after in_use is set to false

    /**
     * @brief Returns the duration in seconds of the currently playing media
     * @return double 
     */
    double get_duration() const; // Thread-Safe
    bool has_media_stream(enum AVMediaType media_type) const; // Thread-Safe


    /**
     * @brief Returns the current timestamp of the video in seconds since the beginning of playback. This takes into account pausing, skipping, etc... and calculates the time according to the current system time given.
     * 
     * 
     * @return The current time of playback since 0:00 in seconds
     */
    double get_time(double current_system_time) const; // Not thread-safe, lock alter_mutex first
    double get_desync_time(double current_system_time) const; // Not thread-safe, lock alter_mutex and audio_buffer_mutex first

    /**
     * @brief Moves the MediaFetcher's playback to a certain time (including video and audio streams)
     * @note The caller is responsible for making sure the time to jump to is in the bounds of the video's playtime. 
     * The video's duration could be found with the MediaFetcher::get_duration() function
     * 
     * @param target_time The target time to jump the playback to (must be reachable)
     * @param current_system_time The current system time
     * @throws If the target time is not in the boudns of the video's playtime
     */
    int jump_to_time(double target_time, double current_system_time); // Not thread-safe, lock alter_mutex and audio_buffer_mutex first

    int load_next_audio(); // Not thread-safe, lock alter_mutex and audio_buffer_mutex first

};



#endif
