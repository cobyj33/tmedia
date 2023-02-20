#ifndef ASCII_VIDEO_PLAYBACK_INCLUDED
#define ASCII_VIDEO_PLAYBACK_INCLUDED

class Playback {
    private:
        bool m_playing;
        double m_speed;
        double m_volume;
        double m_start_time;

        double m_paused_time;
        double m_skipped_time;

        double m_last_pause_time;

    public:

        Playback();


        /**
         * @brief Get the current time (in seconds) of the playback. This time takes into account total time paused, skipped, and played
         * @return double
         */
        double get_time(double current_system_time) const;


        /**
         * @brief Toggle in between resume and stop
         */
        void toggle(double current_system_time);

        /**
         * @brief 
         * @note **Do not use the start function if you are simply trying to resume the playback stream, use the resume function instead**. The start function assumes that the playback is beginning one again at the specified time
         * 
         * @param current_system_time 
         */
        void start(double current_system_time);
        void stop(double current_system_time);

        /**
         * @brief 
         * 
         * @param current_system_time 
         */
        void resume(double current_system_time);
        void skip(double seconds_to_skip);

        bool is_playing() const;
        double get_speed() const;
        double get_volume() const;


        void set_speed(double amount);
        void set_volume(double amount);

        void change_speed(double offset);
        void change_volume(double amount);
};

#endif