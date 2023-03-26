#ifndef ASCII_VIDEO_AUDIO_RESAMPLER_INCLUDE
#define ASCII_VIDEO_AUDIO_RESAMPLER_INCLUDE

#include <vector>

extern "C" {
    #include <libavutil/frame.h>
    #include <libswresample/swresample.h>
}

/**
 * @brief Basic wrapper around FFmpeg's SwrContext, which provides a context for FFmpeg for audio resampling, audio format conversion, and audio rematrixing 
 * 
 */
class AudioResampler {
    private:
        SwrContext* m_context;
        int m_src_sample_rate;
        int m_src_sample_fmt;
        AVChannelLayout* m_src_ch_layout;
        int m_dst_sample_rate;
        int m_dst_sample_fmt;
        AVChannelLayout* m_dst_ch_layout;
    public:
        AudioResampler(AVChannelLayout* dst_ch_layout,
                        enum AVSampleFormat dst_sample_fmt,
                        int dst_sample_rate,
                        AVChannelLayout* src_ch_layout,
                        enum AVSampleFormat src_sample_fmt,
                        int src_sample_rate);

        int get_src_sample_rate();
        int get_src_sample_fmt();
        int get_dst_sample_rate();
        int get_dst_sample_fmt();

        /**
         * @brief Resample an audio frame according to the initialized AudioResampler's parameters
         * 
         * @note The original audio frame is left unaltered.
         * @note Don't forget to free any the original and returned frame after usage by using av_frame_free or another function that frees the data
         * 
         * @param originals The source frame to resample. This source frame is REQUIRED to have the same source channel layout, source sample format, and source sample rate as given to the AudioResampler upon construction
         * @return AVFrame* The resampled frame. This frame WILL have the same destination channel layout, destination sample format, and destination sample rate as given to the AudioResampler upon construction
         */
        AVFrame* resample_audio_frame(AVFrame* original);

        /**
         * @brief Resample a vector of audio frames and return a new set of resampled frames
         * 
         * @note The original list of audio frames is left unaltered.
         * @note Don't forget to free any original or returned frames after resampling and finished
         * 
         * @param originals The frames to resample
         * @return std::vector<AVFrame*> The resampled frames
         */
        std::vector<AVFrame*> resample_audio_frames(std::vector<AVFrame*>& originals);

        /**
         * Frees the SwrContext generated at construction
        */
        ~AudioResampler();
};

#endif