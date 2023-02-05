#include <stdexcept>

#include "audioresampler.h"
#include "except.h"
#include "decode.h"

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/frame.h>
}

AudioResampler::AudioResampler(AVChannelLayout* dst_ch_layout, enum AVSampleFormat dst_sample_fmt, int dst_sample_rate, AVChannelLayout* src_ch_layout, enum AVSampleFormat src_sample_fmt, int src_sample_rate) {
    SwrContext* context = swr_alloc();
    int result = 0;
    result = swr_alloc_set_opts2(
    &context, dst_ch_layout, dst_sample_fmt, 
    dst_sample_rate, src_ch_layout, src_sample_fmt,
    src_sample_rate, 0, nullptr);

    if (result < 0) {
        if (context != nullptr) {
            swr_free(&context);
        }
        throw ascii::bad_alloc("Allocation of internal SwrContext of AudioResampler failed. Aborting...");
    } else {
        result = swr_init(context);
        if (result < 0) {
            if (context != nullptr) {
                swr_free(&context);
            }
            throw std::runtime_error("Initialization of internal SwrContext of AudioResampler failed. Aborting...");
        }
    }

    this->m_context = context;
    this->m_src_sample_rate = src_sample_rate;
    this->m_src_sample_fmt = src_sample_fmt;
    this->m_src_ch_layout = src_ch_layout;
    this->m_dst_sample_rate = dst_sample_rate;
    this->m_dst_sample_fmt = dst_sample_fmt;
    this->m_dst_ch_layout = dst_ch_layout;
}

AudioResampler::~AudioResampler() {
    swr_free(&(this->m_context));
}

std::vector<AVFrame*> AudioResampler::resample_audio_frames(std::vector<AVFrame*>& originals) {
    std::vector<AVFrame*> resampled_frames;

    for (int i = 0; i < originals.size(); i++) {
        AVFrame* resampledFrame = this->resample_audio_frame(originals[i]);
        if (resampledFrame == NULL) {
            clear_av_frame_list(resampled_frames);
            throw std::runtime_error("Unable to resample audio frame " + std::to_string(i) + " in frame list.");
        }
        resampled_frames.push_back(resampledFrame);
    }

    return resampled_frames;
}


AVFrame* AudioResampler::resample_audio_frame(AVFrame* original) {
        int result;
        AVFrame* resampledFrame = av_frame_alloc();
        if (resampledFrame == NULL) {
            throw ascii::bad_alloc("Could not create AVFrame audio frame for resampling");
        }

        resampledFrame->sample_rate = this->m_dst_sample_rate;
        resampledFrame->ch_layout = *(this->m_dst_ch_layout);
        resampledFrame->format = this->m_dst_sample_fmt;
        resampledFrame->pts = original->pts;
        resampledFrame->duration = original->duration;

        result = swr_convert_frame(this->m_context, resampledFrame, original);
        if (result < 0) {
            av_frame_free(&resampledFrame);
            throw std::runtime_error("Unable to resample audio frame");
        }

        return resampledFrame;
}

int AudioResampler::get_src_sample_rate() {
    return this->m_src_sample_rate;
}

int AudioResampler::get_src_sample_fmt() {
    return this->m_src_sample_fmt;
}

int AudioResampler::get_dst_sample_rate() {
    return this->m_dst_sample_rate;
}

int AudioResampler::get_dst_sample_fmt() {
    return this->m_dst_sample_fmt;
}
