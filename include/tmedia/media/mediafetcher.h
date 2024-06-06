#ifndef TMEDIA_MEDIA_FETCHER_H
#define TMEDIA_MEDIA_FETCHER_H

#include <tmedia/media/mediaclock.h>
#include <tmedia/media/mediatype.h>
#include <tmedia/image/pixeldata.h>
#include <tmedia/audio/blocking_audioringbuffer.h>
#include <tmedia/image/scale.h> // for Dim2

#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <array>
#include <thread>
#include <optional>
#include <condition_variable>
#include <filesystem>

extern "C" {
#include <libavutil/avutil.h> // for enum AVMediaType
}

/**
   * Locking Heirarchy:
   * 
   * alter_mutex - General mutations to the MediaFetcher
   * 
   * ex_noti_mtx - Mutex specifically for the exit_cond to notify sleeping threads
   * that the MediaFetcher has been sent an exit dispatch 
   * 
   * resume_notify_mutex - Mutex specifically for the resume_cond to tell sleeping
   * threads that the MediaFetcher has been resumed.
   * 
   * The locking heirarchy for the mutexes specific to condition variables are not as
   * important, as std::scoped_lock can avoid deadlocks anyway if multiple mutexes are
   * passed at once, and cond-paired mutexes should only be used in closed scopes
   * for notifying and receiving a notification
   * 
*/

enum class MediaFetcherFlags {
  VISUALIZE_VIDEO = (1 << 0),
  IGNORE_ATTACHED_PIC = (1 << 1)
};

// Pixel Aspect Ratio - account for tall rectangular shape of terminal
//characters
static constexpr int PAR_WIDTH = 2;
static constexpr int PAR_HEIGHT = 5;

static constexpr int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PAR_HEIGHT;
static constexpr int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PAR_WIDTH;
static constexpr double MAX_FRAME_ASPECT_RATIO = static_cast<double>(MAX_FRAME_ASPECT_RATIO_WIDTH) / static_cast<double>(MAX_FRAME_ASPECT_RATIO_HEIGHT);

// I found that past a width of 640 characters,
// the terminal starts to stutter terribly on most terminal emulators, so we
// just bound the image to this amount

static constexpr int MAX_FRAME_WIDTH = 640;
static constexpr int MAX_FRAME_HEIGHT = static_cast<int>(static_cast<double>(MAX_FRAME_WIDTH) / MAX_FRAME_ASPECT_RATIO);

class MediaFetcher {
  private:
    std::thread video_thread;
    std::thread audio_thread;
    std::thread duration_checking_thread;
    void video_fetching_thread_func();
    void audio_dispatch_thread_func();
    void duration_checking_thread_func();

    void frame_video_fetching_func();
    void frame_image_fetching_func();
    void frame_audio_fetching_func();

    void audio_fetching_thread_func();

    MediaClock clock;
    const std::filesystem::path path;
    
    std::atomic<bool> in_use;
    std::optional<std::string> error;

    int msg_video_jump_curr_time;
    std::mutex ex_noti_mtx;
    std::condition_variable exit_cond;

    std::mutex resume_notify_mutex;
    std::condition_variable resume_cond;
    int msg_audio_jump_curr_time;

  public:
    MediaType media_type;
    std::unique_ptr<BlockingAudioRingBuffer> audio_buffer;
    std::array<bool, AVMEDIA_TYPE_NB> available_streams;
    PixelData frame;
    bool frame_changed;
    int sample_rate;
    int nb_channels;
    double duration;

    std::mutex alter_mutex;
    std::optional<Dim2> req_dims;

    static constexpr int VISUALIZE_VIDEO = 1 << 0;
    static constexpr int IGNORE_ATTACHED_PIC = 1 << 1;
    std::atomic<int> flags;

    MediaFetcher(const std::filesystem::path& path, const std::array<bool, AVMEDIA_TYPE_NB>& requested_streams);

    void begin(double currsystime); // Only to be called by owning thread
    void join(double currsystime); // Only to be called by owning thread after in_use is set to false
    
    /**
     * Thread-Safe
    */
    [[gnu::always_inline]] inline bool has_media_stream(enum AVMediaType media_type) const {
      return this->available_streams[media_type];
    }


    [[gnu::always_inline]] inline bool has_error() const noexcept {
      return this->error.has_value();
    }

    /**
     * Make sure to check with has_error first!
    */
    [[gnu::always_inline]] inline std::string get_error() const {
      return *this->error;
    }

    [[gnu::always_inline]] inline bool should_exit() {
      return !this->in_use;
    }

    void dispatch_exit();
    void dispatch_exit(std::string_view err);

    bool is_playing();
    void pause(double currsystime);
    void resume(double currsystime);


    /**
     * Returns the current timestamp of the video in seconds since the
     * beginning of playback. This takes into account pausing,
     * skipping, etc... and calculates the time according to the
     * current system time given.
     * 
     * Not thread-safe, lock alter_mutex first
     * 
     * @return The current time of playback since 0:00 in seconds
     */
    [[gnu::always_inline]] inline double get_time(double currsystime) const {
      return this->clock.get_time(currsystime);
    }

    /**
     * Returns the absolute difference between the current time of the audio
     * buffer and the expected media time in order to determine the audio desync
     * amount. If there is no audio handled by this MediaFetcher instance,
     * then this function just returns 0.0
     *
     * Not thread-safe, lock alter_mutex first
    */
    double get_audio_desync_time(double currsystime) const;

    /**
     * @brief Moves the MediaFetcher's playback to a certain time (including video and audio streams)
     * @note The caller is responsible for making sure the time to jump to is in the bounds of the video's playtime. 
     * The video's duration could be found with the MediaFetcher::duration
     * 
     * @param target_time The target time to jump the playback to (must be reachable)
     * @param currsystime The current system time
     * @throws If the target time is not in the boudns of the video's playtime
     * 
     * alter_mutex must be locked first before calling for thread safety
     */
    int jump_to_time(double target_time, double currsystime);
};



#endif
