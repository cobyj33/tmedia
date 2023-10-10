
#ifndef ASCII_VIDEO_AUDIO_BUFFER_INCLUDED
#define ASCII_VIDEO_AUDIO_BUFFER_INCLUDED
#include <cstdint>
#include <vector>

/**
 * @brief The audio buffer is a way to store continuous audio data throughout a certain time interval.
 * 
 * The reasoning for a specific AudioBuffer class is to be able to track and pick up exactly where playback leaves off, so as to prevent weird peaking, static, and cuts in audio playback.
 * Additionally, the AudioBuffer object helps easily track the time of the audio playback to detect desynchronization along media playback.
 */
class AudioBuffer {
  private:
    /**
     * @brief The actual buffer of bytes used to store the audio data in the audio buffer.  
     * 
     * All bytes are stored interleavened, which means one audio sample consists of the same number of bytes.
     * 
     * I chose to store audio data as byte unsigned integers instead of floats because the
     * bytes should take 4x less space in memory than floats, and I could not notice a real drop in audio quality.
     * However, the data is converted to a float once it is read by the user
     */
    std::vector<uint8_t> m_buffer;

    /** 
     * @brief the starting time that the first sample in the buffer data is at
     * 
     * This is mainly used in conjunction with the sample rate to find the current time of the playhead in the audio buffer.
    */
    float m_start_time;

    /**
     * @brief The current sample of the playhead in the audio buffer's data. 
     * This idea of the "playhead" is used so that data can continuosly be requested and given again from where the audio buffer previously stopped.
     * 
     * @note This playhead index should NOT take into account the number of channels in the audio data, only the offset of samples since the start of the audio buffer.
     * In order to query into the audio buffer, this value should then be multiplied by the number of audio channels before extracting data
     */
    std::size_t m_playhead;

    int m_nb_channels;

    // The sample rate of the audio stored in this audio buffer
    int m_sample_rate;

  public:
    /**
     * @brief Construct a new audio buffer object with the given number of channels and the given sample rate
     * @param nb_channels The number of channels to be used in the stored audio buffer's data
     * @param sample_rate The sample rate to be used in the stored audio buffer's data
     * @throws Invalid Argument Error if nb_channels or sample_rate is not positive and non-zero
     */
    AudioBuffer(unsigned int nb_channels, unsigned int sample_rate);

    /** gets the number of audio samples stored in the audio buffer */
    std::size_t get_nb_samples() const;
    /** gets the number of audio channels represented by the audio buffer */
    int get_nb_channels() const;
    /** gets the sample rate per second of the data stored by the audio buffer*/
    int get_sample_rate() const;

    /**
     * @brief Clears the audio buffer data and offsets the data to represent it's beginning at a certain "time". This "time" can be something like the time of the start of the buffer in audio or video data.
     * 
     * @param time The "time" that the beginning of the audio buffer represents
     */
    void clear_and_restart_at(double time);

    /** 
     * @brief Clears the audio buffer's data
     * This will reset the nb_samples, 
    */
    void clear();

    /**
     * @brief Get the current time that the audio buffer is on
     * @return The current time of the audio buffer in seconds 
     */
    double get_time() const;

    /**
     * @brief Get the current time that the audio buffer is on, relative to the beginning of the audio buffer's storage
     * @return The current time that has elapsed since the beginning of the audio buffer in seconds
     */
    double get_elapsed_time() const;

    /**
     * @brief Set the time of the audio buffer to a time within the bounds of this audio buffer's loaded time interval
     * 
     * @param time A time within the bounds of this audio buffer's loaded time interval
     * @throws If the time given is not 
     * @note Query for the current time interval with get_start_time and get_end_time, or test if the given time is inside of the interval with is_time_in_bounds before calling this function
     */
    void set_time_in_bounds(double time);

    /**
     * @brief Leave $time seconds of audio data available behind the current playback position
     * 
     * @param time The seconds of audio data that should be left behind the current playback position
     * @throws If the amount of time requested is greater than the amount of time available behind the current buffer's playback position, or if the amount of time requested is negative
     * @note Query for the total available time behind the buffer with $get_elapsed_time to ensure no error is thrown through the above conditions
     */
    void leave_behind(double time);

    /**
     * @brief Gets the starting time of the buffer. The starting time is determined by the time given by clear_and_restart_at
     * @return The time in seconds that the first sample in the buffer represents 
     */
    double get_start_time() const;

    /**
     * @brief Gets the ending time of the buffer. The ending time is determined by the amount of samples inputted into the buffer and the sample rate of the buffer
     * @return The time in seconds that the final sample in the buffer represents 
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
     * @brief Writes nb_samples from a float buffer into the audio buffer. Sample data should be interleavened with each sample consisting of the number of channels currently in use by the audio buffer
     * @param sample_part A part of a sample to write
     */
    void write(float* samples, int nb_samples);

    /**
     * @brief Finds if a certain time stamp lies in the audio buffer's data length
     * 
     * @param time the time to check
     * @return true 
     * @return false 
     */
    bool is_time_in_bounds(double time) const;

    /**
     * @brief Determine if the audio buffer has any sample data to read
     * 
     * @return true: There is any samples available to be read
     * @return false: There is not samples available to read
     */
    bool can_read() const;

    /**
     * @brief Determine if the audio buffer has enough data to be able to read nb_samples number of samples
     * This does not necessarily mean that the buffer is empty, only that there is no further data to be found or all data has previously been read.
     * 
     * @param nb_samples The number of samples to test if able to be read
     * @return true: There is enough samples to be able to read
     * @return false: There is not enough samples able to be read
     */
    bool can_read(std::size_t nb_samples) const;

    std::size_t get_nb_can_read() const;

    /**
     * @brief Reads nb_samples samples into the float buffer and advanced the audio buffer by nb_samples. The data returned is interleaved
     * The float buffer should be of the size (nb_samples * nb_channels), where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio buffer. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio buffer through get_nb_channels()
     * NOTE: This function advances the audio buffer, so multiple calls to read_into in a row will not read the same data.
     * 
     * @param nb_samples The number of samples to be read into the buffer
     * @param target The float buffer to read the data into. The buffer should be of a length of at least (nb_samples * nb_channels) where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio buffer. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio buffer through get_nb_channels()
     */
    void read_into(std::size_t nb_samples, float* target);

    /**
     * @brief Reads nb_samples samples into the float buffer. The data returned is interleaved
     * The float buffer should be of the size (nb_samples * nb_channels), where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio buffer. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio buffer through get_nb_channels()
     * NOTE: This function is unlike read_into(nb_samples, float* target). In peek_into, the audio buffer is not automatically advanced by nb_samples
     * 
     * @param nb_samples The number of samples to be read into the buffer
     * @param target The float buffer to read the data into. The buffer should be of a length of at least (nb_samples * nb_channels) where nb_samples is the number of samples wanted to be read and nb_channels is the number of channels in the audio buffer. This can be easily found by multiplying the nb_samples to be read by the number of channels in the audio buffer through get_nb_channels()
     */
    void peek_into(std::size_t nb_samples, float* target) const;
    std::vector<float> peek_into(std::size_t nb_samples);

    /**
     * @brief Moves the audio buffer forward by nb_samples amount
     * @param nb_samples the number of samples to move the audio buffer
     */
    void advance(std::size_t nb_samples);
};

#endif