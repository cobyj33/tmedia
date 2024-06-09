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

// Pixel Aspect Ratio - account for tall rectangular shape of terminal
//characters
static constexpr int PAR_WIDTH = 2;
static constexpr int PAR_HEIGHT = 5;

static constexpr int MAX_FRAME_ASPECT_RATIO_WIDTH = 16 * PAR_HEIGHT;
static constexpr int MAX_FRAME_ASPECT_RATIO_HEIGHT = 9 * PAR_WIDTH;
static constexpr double MAX_FRAME_ASPECT_RATIO = static_cast<double>(MAX_FRAME_ASPECT_RATIO_WIDTH) / static_cast<double>(MAX_FRAME_ASPECT_RATIO_HEIGHT);

/**
 * A width of 640 at a 4:3 aspect ratio ends up around 480p. Additionally,
 * I find that past a width of 640 characters,
 * the terminal starts to stutter terribly on most terminal emulators, and CPU
 * usage becomes extremely high, so we
 * just bound the image to this amount.
 * 
 * This number can just be configured to any maximum amount wanted during
 * compilation. Currently, this value is not changeable at runtime, but there
 * are considerations for allowing that behavior. 
 */
static constexpr int MAX_FRAME_WIDTH = 640;
static constexpr int MAX_FRAME_HEIGHT = static_cast<int>(static_cast<double>(MAX_FRAME_WIDTH) / MAX_FRAME_ASPECT_RATIO);

static_assert(MAX_FRAME_WIDTH > 0);
static_assert(MAX_FRAME_HEIGHT > 0);
static_assert(PAR_WIDTH > 0);
static_assert(PAR_HEIGHT > 0);
static_assert(MAX_FRAME_ASPECT_RATIO_WIDTH > 0);
static_assert(MAX_FRAME_ASPECT_RATIO_HEIGHT > 0);
static_assert(MAX_FRAME_ASPECT_RATIO > 0.0);

/**
 * The MediaFetcher is the main class to coordinate media playback in tmedia.
 * 
 * 
 * Simply, the usage of the MediaFetcher consists of initializing
 * the MediaFetcher in its constructor, calling begin() to fire the threads,
 * looping until MediaFetcher::should_exit() becomes true, calling join()
 * to join all of the finished threads to the main thread, 
 * 
 * A MediaFetcher can only be configured to fetch and playback a single
 * media file instance, it cannot be reconfigured to be used again for playing
 * back multiple different files. A different MediaFetcher instance must
 * be initialized and used in such a case.
 */
class MediaFetcher {
  private:
    std::thread video_thread;
    std::thread audio_thread;
    std::thread duration_checking_thread;
    void video_fetching_thread_func(); // only for use by video_thread
    void audio_dispatch_thread_func(); // only for use by audio_thread
    void duration_checking_thread_func(); // only for use by duration_checking_thread

    /**
     * Separate functions for processing different types of media owned
     * by the MediaFetcher instance. These functions are used in the
     * video thread to process frames to MediaFetcher::frame.
     */

    void frame_video_fetching_func(); // only for use by video_thread
    void frame_image_fetching_func(); // only for use by video_thread
    void frame_audio_fetching_func(); // only for use by video_thread

    void audio_fetching_thread_func(); // only for use by audio_thread

    MediaClock clock; // Not thread safe, lock alter_mutex first before access
    const std::filesystem::path path; // Thread Safe, immutable
    
    /**
     * Thread Safe, Atomic
     * 
     * A flag denoting if the MediaFetcher instance is still in use or if all
     * threads should exit operating on it.
     */
    std::atomic<bool> in_use;
    std::optional<std::string> error; // Thread Safe, immutable

    int msg_video_jump_curr_time; // Not thread safe, lock alter_mutex first before access
    std::mutex ex_noti_mtx;
    std::condition_variable exit_cond;

    std::mutex resume_notify_mutex;
    std::condition_variable resume_cond;
    int msg_audio_jump_curr_time; // Not thread safe, lock alter_mutex first before access

  public:
    MediaType media_type; // Thread Safe, immutable
    std::unique_ptr<BlockingAudioRingBuffer> audio_buffer; // Thread Safe, uses internal locks
    std::array<bool, AVMEDIA_TYPE_NB> available_streams; // Thread Safe, immutable
    PixelData frame; // Not thread safe, lock alter_mutex first before access
    bool frame_changed; // Not thread safe, lock alter_mutex first before access


    int sample_rate; // Thread Safe, immutable
    int nb_channels; // Thread Safe, immutable
    double duration; // Thread Safe, immutable

    /**
     * Main mutex to share 
     */
    std::mutex alter_mutex;
    std::optional<Dim2> req_dims;

    MediaFetcher(const std::filesystem::path& path, const std::array<bool, AVMEDIA_TYPE_NB>& requested_streams);

    /**
     * Begin the threads of the MediaFetcher, tracking the current system time
     * as the beginning of playback.
     * Must only be called by the owning thread.
     */
    void begin(double currsystime); // Only to be called by owning thread
    
    /**
     * Join all spawned threads by the MediaFetcher back to the calling thread.
     * This should only be called whenever media playback has finished, signaled
     * through MediaFetcher::should_exit() returning true.
     * Must only be called by the owning thread.
     */ 
    void join(double currsystime); // Only to be called by owning thread after in_use is set to false
    
    /**
     * Thread-Safe: Immutable
    */
    [[gnu::always_inline]] inline bool has_media_stream(enum AVMediaType media_type) const {
      return this->available_streams[media_type];
    }

    /**
     * Not Thread-Safe: Lock alter_mutex first.
    */
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

    /**
     * Thread-Safe
     * 
     * ! DO NOT CALL WITH ex_noti_mtx or resume_notify_mutex locked!
     * This function locks both, and if they are already locked it will cause
     * undefined behavior.
     * Since ex_noti_mtx and resume_notify_mutex are private, this is not an
     * issue for the owner of a MediaFetcher object, but rather the
     * implementation of the MediaFetcher
     */
    void dispatch_exit(); // Thread-Safe

    /**
     * Not Thread Safe: Lock alter_mutex
     * 
     * ! DO NOT CALL WITH ex_noti_mtx or resume_notify_mutex locked!
     * This function locks both, and if they are already locked it will cause
     * undefined behavior.
     * Since ex_noti_mtx and resume_notify_mutex are private, this is not an
     * issue for the owner of a MediaFetcher object, but rather the
     * implementation of the MediaFetcher
     * 
     * NOTE:
     * alter_mutex must be locked first before calling for thread safety
     */
    void dispatch_exit_err(std::string_view err); // Not Thread Safe: Lock alter_mutex

    bool is_playing(); // Thread-Safe: Atomic

    /**
     * Not Thread Safe: Lock alter_mutex
     * 
     * Pauses playback of the current media streams.
     */
    void pause(double currsystime);

    /**
     * Not Thread Safe: Lock alter_mutex
     * 
     * Resumes playback if paused
     */
    void resume(double currsystime);


    /**
     * Returns the current timestamp of the video in seconds since the
     * beginning of playback. This takes into account pausing,
     * skipping, etc... and calculates the time according to the
     * current system time given.
     * 
     * @return The current time of playback since 0:00 in seconds
     * 
     * NOTE:
     * Not thread-safe, lock alter_mutex first
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
     * NOTE:
     * alter_mutex must be locked first before calling for thread safety
    */
    double get_audio_desync_time(double currsystime) const;

    /**
     * @brief Moves the MediaFetcher's playback to a certain time
     * (including video and audio streams)
     * 
     * @note The caller is responsible for making sure the time to jump to is
     * in the bounds of the video's playtime.
     * 
     * The video's duration could be found with the MediaFetcher::duration
     * 
     * @param target_time The target time to jump the playback to
     * (must be reachable).
     * It is recommended to simply clamp the requested time between 0 and
     * MediaFetcher::duration whenever calling this function for guaranteed
     * safety.
     * @param currsystime The current system time
     * @throws If the target time is not in the boudns of the video's playtime
     * 
     * NOTE:
     * alter_mutex must be locked first before calling for thread safety
     */
    int jump_to_time(double target_time, double currsystime);
};



#endif
