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
        static constexpr double MAX_VOLUME = 2.0;
        static constexpr double MIN_VOLUME = 0.0;

        static constexpr double MAX_SPEED = 2;
        static constexpr double MIN_SPEED = 0.5;

        Playback();


        /**
         * @brief Get the current time (in seconds) of the playback. This time takes into account total time paused, skipped, and played
         * @return double
         */
        double get_time(double current_system_time);


        /**
         * @brief Toggle in between resume and stop
         */
        void toggle(double current_system_time);
        void start(double current_system_time);
        void stop(double current_system_time);
        void resume(double current_system_time);
        void skip(double seconds_to_skip);

        bool is_playing();
        double get_speed();
        double get_volume();


        void set_playing(bool playing);
        void set_speed(double amount);
        void set_volume(double amount);

        void change_speed(double offset);
        void change_volume(double amount);
};

#endif