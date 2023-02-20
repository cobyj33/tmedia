#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "decode.h"
#include "except.h"
#include "audioresampler.h"

extern "C" {
#include <libavutil/error.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
#include <libavfilter/avfilter.h>
}

void clear_av_frame_list(std::vector<AVFrame*>& frame_list) {
    for (int i = 0; i < frame_list.size(); i++) {
        av_frame_free(&(frame_list[i]));
    }
    frame_list.clear();
}


std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet) {
    std::vector<AVFrame*> video_frames;
    int result;

    result = avcodec_send_packet(video_codec_context, video_packet);
    if (result < 0 && result != AVERROR(EAGAIN)) {
        return video_frames;
        // throw ascii::ffmpeg_error("Error while sending video packet: ", result);
    }

    AVFrame* video_frame = av_frame_alloc();

    while (result == 0) {
        result = avcodec_receive_frame(video_codec_context, video_frame);

        if (result < 0) {
            if (result == AVERROR(EAGAIN)) {
                break;
            } else {
                av_frame_free(&video_frame);
                clear_av_frame_list(video_frames);
                return video_frames;
                // throw ascii::ffmpeg_error("Error while receiving video frames during decoding:", result);
            }
        }

        AVFrame* saved_frame = av_frame_alloc();
        result = av_frame_ref(saved_frame, video_frame);
        
        if (result < 0) {
            av_frame_free(&saved_frame);
            av_frame_free(&video_frame);
            clear_av_frame_list(video_frames);
            return video_frames;

            // throw ascii::ffmpeg_error("ERROR while referencing video frames during decoding:", result);
        }

        video_frames.push_back(saved_frame);
        av_frame_unref(video_frame);
    }
    
    av_frame_free(&video_frame);
    return video_frames;
}




std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet) {
    std::vector<AVFrame*> audio_frames;
    int result;

    result = avcodec_send_packet(audio_codec_context, audio_packet);
    if (result < 0) {
        throw ascii::ffmpeg_error("ERROR WHILE SENDING AUDIO PACKET ", result);
    }

    AVFrame* audio_frame = av_frame_alloc();
    result = avcodec_receive_frame(audio_codec_context, audio_frame);
    if (result < 0) { // if EAGAIN, caller should catch and send more data
        av_frame_free(&audio_frame);
        throw ascii::ffmpeg_error("FATAL ERROR WHILE RECEIVING AUDIO FRAME ", result);
    }

    while (result == 0) {
        AVFrame* saved_frame = av_frame_alloc();
        result = av_frame_ref(saved_frame, audio_frame); 
        if (result < 0) {
            clear_av_frame_list(audio_frames);
            av_frame_free(&saved_frame);
            av_frame_free(&audio_frame);
            throw ascii::ffmpeg_error("ERROR WHILE REFERENCING AUDIO FRAMES DURING DECODING ", result);
        }

        av_frame_unref(audio_frame);
        audio_frames.push_back(saved_frame);
        result = avcodec_receive_frame(audio_codec_context, audio_frame);
    }

    av_frame_free(&audio_frame);
    return audio_frames;
}

std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, PlayheadList<AVPacket*>& video_packet_list) {
    std::vector<AVFrame*> decoded_frames;

    while (video_packet_list.can_step_forward()) {
        try {
            decoded_frames = decode_video_packet(video_codec_context, video_packet_list.get());
            if (decoded_frames.size() == 0 && video_packet_list.can_step_forward()) {
                video_packet_list.step_forward();
                continue;
            }

            return decoded_frames;
        } catch (ascii::ffmpeg_error e) {
            if (e.is_eagain() && video_packet_list.can_step_forward()) {
                video_packet_list.step_forward();
            } else {
                throw e;
            }
        }
    }
    
    return decoded_frames;
}

void decode_until(AVCodecContext* video_codec_context, PlayheadList<AVPacket*>& video_packet_list, int64_t target_pts) {
    while (video_packet_list.get()->pts < target_pts && video_packet_list.can_step_forward()) {
        decode_video_packet(video_codec_context, video_packet_list);
        video_packet_list.try_step_forward();
    }
}


std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, PlayheadList<AVPacket*>& audio_packet_list) {
    std::vector<AVFrame*> decoded_frames;

    while (audio_packet_list.can_step_forward()) {
        try {
            decoded_frames = decode_audio_packet(audio_codec_context, audio_packet_list.get());
            if (decoded_frames.size() == 0 && audio_packet_list.can_step_forward()) {
                audio_packet_list.step_forward();
                continue;
            }

            return decoded_frames;
        } catch (ascii::ffmpeg_error e) {
            if (e.is_eagain() && audio_packet_list.can_step_forward()) {
                audio_packet_list.step_forward();
            } else {
                throw e;
            }
        }
    }

    return decoded_frames;
}

std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audio_codec_context, AudioResampler& audio_resampler, AVPacket* packet) {
    std::vector<AVFrame*> raw_audio_frames = decode_audio_packet(audio_codec_context, packet);
    std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(raw_audio_frames);
    clear_av_frame_list(raw_audio_frames);
    return audio_frames;
}

std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audio_codec_context, AudioResampler& audio_resampler, PlayheadList<AVPacket*>& packet_buffer) {
    std::vector<AVFrame*> raw_audio_frames = decode_audio_packet(audio_codec_context, packet_buffer);
    std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(raw_audio_frames);
    clear_av_frame_list(raw_audio_frames);
    return audio_frames;
}
