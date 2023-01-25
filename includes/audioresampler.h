#ifndef ASCII_VIDEO_AUDIO_RESAMPLER_INCLUDE
#define ASCII_VIDEO_AUDIO_RESAMPLER_INCLUDE

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


        AVFrame* resample_audio_frame(AVFrame* original);
        AVFrame** resample_audio_frames(AVFrame** originals, int nb_frames);

        ~AudioResampler();
};

#endif