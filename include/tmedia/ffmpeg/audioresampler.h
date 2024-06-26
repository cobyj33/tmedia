#ifndef TMEDIA_AUDIO_RESAMPLER_H
#define TMEDIA_AUDIO_RESAMPLER_H

#include <vector>

#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/util/defines.h>

extern "C" {
  #include <libavutil/frame.h>
  #include <libswresample/swresample.h>
  #include <libavutil/channel_layout.h>
}

/**
 * Basic RAII wrapper around FFmpeg's SwrContext
 * 
 * Used to easily convert audio AVFrame structs between different sample rates,
 * sample formats, and channel layouts
 */
class AudioResampler {
  private:
    SwrContext* m_context;
    int m_src_sample_rate;
    int m_dst_sample_rate;

    int m_src_sample_fmt;
    int m_dst_sample_fmt;
    #if HAS_AVCHANNEL_LAYOUT
    AVChannelLayout m_src_ch_layout;
    AVChannelLayout m_dst_ch_layout;
    #else
    int64_t m_src_ch_layout;
    int64_t m_dst_ch_layout;
    #endif
  public:
    #if HAS_AVCHANNEL_LAYOUT
    AudioResampler(AVChannelLayout* dst_ch_layout,
            enum AVSampleFormat dst_sample_fmt,
            int dst_sample_rate,
            AVChannelLayout* src_ch_layout,
            enum AVSampleFormat src_sample_fmt,
            int src_sample_rate);
    #else
    AudioResampler(int64_t dst_ch_layout,
            enum AVSampleFormat dst_sample_fmt,
            int dst_sample_rate,
            int64_t src_ch_layout,
            enum AVSampleFormat src_sample_fmt,
            int src_sample_rate);
    #endif

    TMEDIA_ALWAYS_INLINE inline int get_src_sample_rate() { return this->m_src_sample_rate; }
    TMEDIA_ALWAYS_INLINE inline int get_src_sample_fmt() { return this->m_src_sample_fmt; }
    TMEDIA_ALWAYS_INLINE inline int get_dst_sample_rate() { return this->m_dst_sample_rate; }
    TMEDIA_ALWAYS_INLINE inline int get_dst_sample_fmt() { return this->m_dst_sample_fmt; } 

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
     * @brief Resample a vector of audio AVFrames and return a new set of resampled frames
     * 
     * @note The original list of audio frames is left unaltered.
     * @note Don't forget to free any original or returned frames after resampling and finished
     * 
     * @param originals The AVFrames to resample
     * @return std::vector<AVFrame*> The resampled frames
     */
    std::vector<AVFrame*> resample_audio_frames(std::vector<AVFrame*>& originals);

    ~AudioResampler();
};

#endif