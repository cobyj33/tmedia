#ifndef TMEDIA_MEDIA_CLOCK_H
#define TMEDIA_MEDIA_CLOCK_H

#include <atomic>

/**
 * Timestamp driven media clock which calculates elapsed time based on changes
 * in timestamps. 
 * 
 * When using a system clock, a monotonic clock should be used to track media
 * playblack, as constant forward clock motion will best model the movement of
 * media playback
*/
class MediaClock {
  private:
    std::atomic<bool> m_playing;
    double m_start_time;

    double m_paused_time;
    double m_skipped_time;

    double m_last_pause_system_time;

  public:

    MediaClock();

    /**
     * @brief Get the current time (in seconds) of the clock. This time takes into account total time paused, skipped, and played
     * @param current_system_time The current system time
     * @return double
     */
    double get_time(double current_system_time) const;

    /**
     * @brief Toggle in between resume and stop
     */
    void toggle(double current_system_time);

    /**
     * @note **Do not use the init function if you are simply trying to resume the clock stream, use the resume function instead**.
     * The init function assumes that the clock is beginning once again at the specified time
     * 
     * @param current_system_time 
     */
    void init(double current_system_time);
    void stop(double current_system_time);

    /**
     * @brief 
     * 
     * @param current_system_time 
     */
    void resume(double current_system_time);
    void skip(double seconds_to_skip);

    bool is_playing() const;
};

#endif