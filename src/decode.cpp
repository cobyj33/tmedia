#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include <deque>

#include "decode.h"
#include "except.h"
#include "audioresampler.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

void clear_av_frame_list(std::vector<AVFrame*>& frame_list) {
    for (int i = 0; i < frame_list.size(); i++) {
        av_frame_free(&(frame_list[i]));
    }
    frame_list.clear();
}

void decode_video_packet_void(AVCodecContext* video_codec_context, AVPacket* video_packet) {
    int result;

    result = avcodec_send_packet(video_codec_context, video_packet);
    if (result < 0) {
        return;
    }

    AVFrame* video_frame = av_frame_alloc();
    while (result == 0) {
        result = avcodec_receive_frame(video_codec_context, video_frame);
    }
    av_frame_free(&video_frame);
}


std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet) {
    std::vector<AVFrame*> video_frames;
    int result;

    result = avcodec_send_packet(video_codec_context, video_packet);
    if (result < 0) {
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

std::vector<AVFrame*> decode_video_packet_queue(AVCodecContext* video_codec_context, std::deque<AVPacket*>& video_packet_queue) {
    std::vector<AVFrame*> decoded_frames;
    AVPacket* packet;
    
    while (!video_packet_queue.empty()) {
        try {
            packet = video_packet_queue.front();
            video_packet_queue.pop_front();
            decoded_frames = decode_video_packet(video_codec_context, packet);
            av_packet_free(&packet);

            if (decoded_frames.size() == 0 && !video_packet_queue.empty()) {
                continue;
            }
            return decoded_frames;
        } catch (ascii::ffmpeg_error e) {
            av_packet_free(&packet);
            if (!e.is_eagain() || video_packet_queue.empty()) { // if error is fatal, or the packet list is empty
                throw e;
            }
        }
    }
    
    return decoded_frames;
}

void decode_video_packet_queue_until(AVCodecContext* video_codec_context, std::deque<AVPacket*>& video_packet_queue, int64_t target_pts) {
    while (video_packet_queue.front()->pts < target_pts && !video_packet_queue.empty()) {
        AVPacket* packet = video_packet_queue.front();
        video_packet_queue.pop_front();
        decode_video_packet_void(video_codec_context, packet);
        av_packet_free(&packet);
    }
}


std::vector<AVFrame*> decode_audio_packet_queue(AVCodecContext* audio_codec_context, std::deque<AVPacket*>& audio_packet_queue) {
    std::vector<AVFrame*> decoded_frames;
    AVPacket* packet;

    while (!audio_packet_queue.empty()) {
        try {
            packet = audio_packet_queue.front();
            audio_packet_queue.pop_front();
            decoded_frames = decode_audio_packet(audio_codec_context, packet);
            av_packet_free(&packet);
            if (decoded_frames.size() == 0 && !audio_packet_queue.empty()) {
                continue;
            }

            return decoded_frames;
        } catch (ascii::ffmpeg_error e) {
            av_packet_free(&packet);
            if (!e.is_eagain() || audio_packet_queue.empty()) { // if error is fatal, or the packet list is empty
                throw e;
            }
        }
    }

    return decoded_frames;
}