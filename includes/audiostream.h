
#ifndef ASCII_VIDEO_AUDIO_STREAM_INCLUDED
#define ASCII_VIDEO_AUDIO_STREAM_INCLUDED
#include <cstdint>
#include <vector>

/**
 * @brief The audio stream is a way to store continuous audio data throughout a certain time interval.
 * 
 * The reasoning for a specific AudioStream class is to be able to track and pick up exactly where playback leaves off, so as to prevent weird peaking, static, and cuts in audio playback.
 * Additionally, the AudioStream object helps easily track the time of the audio playback to detect desynchronization along media playback.
 * 
 * While this structure is very similar to the PlayheadList, as it has a buffer of data with a "playhead" index, the AudioStream allows much faster indexing and fetching of data, as the data is stored in a vector instead of a doubly-linked list  
 */
class AudioStream {
    private:
        /**
         * @brief The actual stream of bytes used to store the audio data in the audio stream.
         * All bytes are stored interleavened, which means one audio sample consists of the same number of bytes.
         * I chose to store audio data as byte unsigned integers instead of floats because the bytes should take 4x less space in memory than floats. However, the data is converted to a float once it is read by the user
         * 
         */
        std::vector<uint8_t> m_stream;

        /** 
         * @brief the starting time that the first sample in the stream data is at
         * 
         * This is mainly used in conjunction with the sample rate to find the current time of the playhead in the audio stream
        */
        float m_start_time;

        /**
         * @brief The current sample of the playhead in the audio stream's data. This idea of the "playhead" is used so that data can continuosly be requested and given again from where the audio stream previously stopped.
         */
        std::size_t m_playhead;

        int m_nb_channels;

        // The sample rate of the audio stored in this audio stream
        int m_sample_rate;

        /**
         * @brief Whether the audio stream is initialized or not. The audio stream must be initialized in order to begin storing audio data. An audio stream only needs to be initialized once.
         */
        bool m_initialized;
        
    void construct();

    public:
        /**
         * @brief Creates an empty uninitialized audio stream object
        */
        AudioStream();

        /**
         * @brief Construct a new initialized audio stream object with the given number of channels and the given sample rate
         * @param nb_channels The number of channels to be used in the stored audio stream's data
         * @param sample_rate The sample rate to be used in the stored audio stream's data
         * @throws Invalid Argument Error if nb_channels or sample_rate is not positive and non-zero
         */
        AudioStream(int nb_channels, int sample_rate);

        /** gets the number of audio samples stored in the audio stream */
        std::size_t get_nb_samples() const;
        /** gets the number of audio channels represented by the audio stream */
        int get_nb_channels() const;
        /** gets the sample rate per second of the data stored by the audio stream*/
        int get_sample_rate() const;

        /**
         * @brief Clears the audio stream data and offsets the data to represent it's beginning at a certain "time". This "time" can be something like the time of the start of the stream in audio or video data.
         * 
         * @param time The "time" that the beginning of the audio stream represents
         */
        void clear_and_restart_at(double time);

        /** 
         * @brief Clears the audio stream's data
         * This will reset the nb_samples, 
        */
        void clear();

        /**
         * @brief Initialize the audio stream with a given number of channels and sample rate to store it's data
         * NOTE: If the audio data is reinitialized, the audio stream data will be cleared. It's recommended to create a new AudioStream object instead of reinitializing an old one
         * 
         * @param nb_channels The number of channels to be used in the stored audio stream's data
         * @param sample_rate The sample rate to be used in the stored audio stream's data
         * @throws Invalid Argument Error: if nb_channels or sample_rate is not positive and non-zero
         */
        void init(int nb_channels, int sample_rate);


        /**
         * @brief Get the current time that the audio stream is on
         * @return The current time of the audio stream in seconds 
         */
        double get_time() const;

        /**
         * @brief Get the current time that the audio stream is on
         * @return The current time of the audio stream in seconds 
         */
        double get_elapsed_time() const;
        std::size_t set_time(double time); // returns new playhead position

        /**
         * @brief Gets the ending time of the stream. The ending time is determined by the amount of samples inputted into the stream and the sample rate of the stream
         * @return The time in seconds that the final sample in the stream represents 
         */
        double get_end_time() const;

        /**
         * @brief Writes a part of a sample into the audio data
         * @param sample_part A part of a sample to write
         */
        void write(uint8_t sample_part);

        /**
         * @brief Writes a part of a sample into the audio data
         * @param sample_part A part of a sample to write
         */
        void write(float sample_part);

        /**
         * @brief Writes nb_samples from a float buffer into the audio stream. Sample data should be interleavened with each sample consisting of the number of channels currently in use by the audio stream
         * @param sample_part A part of a sample to write
         */
        void write(float* samples, int nb_samples);

        /**
         * @brief Finds if a certain time stamp lies in the audio stream's data length
         * 
         * @param time the time to check
         * @return true 
         * @return false 
         */
        bool is_time_in_bounds(double time) const;

        /**
         * @brief Determine if the audio stream has any sample data to read
         * 
         * @return true: There is any samples available to be read
         * @return false: There is not samples available to read
         */
        bool can_read() const;

        /**
         * @brief Determine if the audio stream has enough data to be able to read nb_samples number of samples
         * This does not necessarily mean that the stream is empty, only that there is no further data to be found or all data has previously been read.
         * 
         * @param nb_samples The number of samples to test if able to be read
         * @return true: There is enough samples to be able to read
         * @return false: There is not enough samples able to be read
         */
        bool can_read(std::size_t nb_samples) const;

        std::size_t get_nb_can_read() const;

        /**
         * @brief Reads nb_samples samples into the float buffer and advanced the audio stream by nb_samples. The data returned is interleaved
         * The float buffer should be of the size (nb_samples * nb_channels), where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio stream. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio stream through get_nb_channels()
         * NOTE: This function advances the audio stream, so multiple calls to read_into in a row will not read the same data.
         * 
         * @param nb_samples The number of samples to be read into the buffer
         * @param target The float buffer to read the data into. The buffer should be of a length of at least (nb_samples * nb_channels) where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio stream. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio stream through get_nb_channels()
         */
        void read_into(std::size_t nb_samples, float* target);

        /**
         * @brief Reads nb_samples samples into the float buffer. The data returned is interleaved
         * The float buffer should be of the size (nb_samples * nb_channels), where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio stream. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio stream through get_nb_channels()
         * NOTE: This function is unlike read_into(nb_samples, float* target). In peek_into, the audio stream is not automatically advanced by nb_samples
         * 
         * @param nb_samples The number of samples to be read into the buffer
         * @param target The float buffer to read the data into. The buffer should be of a length of at least (nb_samples * nb_channels) where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio stream. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio stream through get_nb_channels()
         */
        void peek_into(std::size_t nb_samples, float* target) const;

        /**
         * @brief Moves the audio stream forward by nb_samples amount
         * @param nb_samples the number of samples to move the audio stream
         */
        void advance(std::size_t nb_samples);

        /**
         * @brief Determines whether the audio stream has been initialized with 
         * 
         * @return true: The audio stream has been initialized and is ready to store data
         * @return false: The audio stream has not been initialized and data cannot yet be written to it
         */
        bool is_initialized() const;
};

#endif