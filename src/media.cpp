
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <algorithm>
#include <memory>

#include "boiler.h"
#include "playheadlist.hpp"
#include "media.h"
#include "wtime.h"
#include "wmath.h"
#include "except.h"
#include "formatting.h"
#include "threads.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

MediaStream& MediaData::get_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->nb_streams; i++) {
        if (this->media_streams[i]->get_media_type() == media_type) {
            return *this->media_streams[i];
        }
    }
    throw ascii::not_found_error("Cannot get media stream " + std::string(av_get_media_type_string(media_type)) + " from media data, could not be found");
}

bool MediaData::has_media_stream(enum AVMediaType media_type) {
    try {
        this->get_media_stream(media_type);
        return true;
    } catch (ascii::not_found_error not_found_error) {
        return false;
    }
}


void move_packet_list_to_pts(PlayheadList<AVPacket*>& packets, int64_t targetPTS) {
    if (packets.is_empty()) {
        return;
    }

    int64_t lastPTS = packets.get()->pts;
    while (true) {

        if (packets.get()->pts == targetPTS) {
            break;
        } else if  (std::min(packets.get()->pts, lastPTS) <= targetPTS && std::max(packets.get()->pts, lastPTS) >= targetPTS) {
            break;
        } else if (packets.get()->pts > targetPTS) {
            if (packets.can_step_backward()) {
                packets.step_backward();
                if (packets.get()->pts <= targetPTS) {
                    break;
                }
            } else {
                break;
            }
        } else if (packets.get()->pts < targetPTS) {
            if (packets.can_step_forward()) {
                packets.step_forward();
                if (packets.get()->pts == targetPTS) {
                    break;
                } else if (packets.get()->pts > targetPTS) {
                    packets.step_backward();
                    break;
                }
            } else {
                break;
            }
        }
        
        lastPTS = packets.get()->pts;
    }
}



void move_frame_list_to_pts(PlayheadList<AVFrame*>& frames, int64_t targetPTS) {
    if (frames.is_empty()) {
        return;
    }

    int64_t lastPTS = frames.get()->pts;
    while (true) {

        if (frames.get()->pts == targetPTS) {
            break;
        } else if  (std::min(frames.get()->pts, lastPTS) <= targetPTS && std::max(frames.get()->pts, lastPTS) >= targetPTS) {
            break;
        } else if (frames.get()->pts > targetPTS) {
            if (frames.can_step_backward()) {
                frames.step_backward();
                if (frames.get()->pts <= targetPTS) {
                    break;
                }
            } else {
                break;
            }
        } else if (frames.get()->pts < targetPTS) {
            if (frames.can_step_forward()) {
                frames.step_forward();
                if (frames.get()->pts == targetPTS) {
                    break;
                } else if (frames.get()->pts > targetPTS) {
                    frames.step_backward();
                    break;
                }
            } else {
                break;
            }
        }
        
        lastPTS = frames.get()->pts;
    }
}


double MediaPlayer::get_duration() const {
    return this->media_data->duration;
}

bool MediaPlayer::has_video() const {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_VIDEO);
}
bool MediaPlayer::has_audio() const {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_AUDIO);
}

MediaStream& MediaPlayer::get_video_stream() const {
    if (this->has_video()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);
    }
    throw std::runtime_error("Cannot get video stream from MediaPlayer, no video is available to retrieve");
}

MediaStream& MediaPlayer::get_audio_stream() const {
    if (this->has_audio()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_AUDIO);
    }
    throw std::runtime_error("Cannot get audio stream from MediaPlayer, no audio is available to retrieve");
}

double MediaPlayer::get_desync_time(double current_system_time) const {
    if (this->has_audio() && this->has_video()) {
        double current_playback_time = this->playback.get_time(current_system_time);
        double desync = std::abs(this->cache.audio_stream.get_time() - current_playback_time);
        return desync;
    } else {
        return 0.0;
    }
}

void MediaPlayer::resync(double current_system_time) {

    if (this->has_audio()) {
        AudioStream& audio_stream = this->cache.audio_stream;
        MediaStream& audio_media_stream = this->get_audio_stream();


        double current_playback_time = this->playback.get_time(current_system_time);
        if (audio_stream.is_time_in_bounds(current_playback_time)) {
            audio_stream.set_time(current_playback_time);
        } else {
            audio_stream.clear_and_restart_at(current_playback_time);
        }

        move_packet_list_to_pts(audio_media_stream.packets, current_playback_time / audio_media_stream.get_time_base());
    }
}

void MediaPlayer::set_current_image(PixelData& data) {
    this->cache.image = PixelData(data);
}

void MediaPlayer::set_current_image(AVFrame* frame) {
    this->cache.image = PixelData(frame);
}


PixelData& MediaPlayer::get_current_image() {
    return this->cache.image;
}

MediaData::MediaData(const char* file_name) {
    int result;

    try {
        this->format_context = open_format_context(file_name);
    } catch (std::runtime_error e) {
        throw std::runtime_error("Could not allocate media data of " + std::string(file_name) + " because of error while fetching file format data: " + e.what());
    } 

    enum AVMediaType media_types[2] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    this->stream_datas = std::make_unique<StreamDataGroup>(this->format_context, media_types, 2);

    this->allPacketsRead = false;
    this->nb_streams = this->stream_datas->get_nb_streams();

    for (int i = 0; i < this->nb_streams; i++) {
        std::unique_ptr<MediaStream> media_stream_ptr = std::make_unique<MediaStream>( (*this->stream_datas)[i] );
        this->media_streams.push_back(std::move(media_stream_ptr));
    }

    this->duration = (double)this->format_context->duration / AV_TIME_BASE;
}

double MediaStream::get_average_frame_rate_sec() const {
    return av_q2d(this->info.stream->avg_frame_rate);
}

double MediaStream::get_average_frame_time_sec() const {
    return 1 / av_q2d(this->info.stream->avg_frame_rate);
}

double MediaStream::get_start_time() const {
    return this->info.stream->start_time * this->get_time_base();
}

int MediaStream::get_stream_index() const {
    return this->info.stream->index;
}

void MediaStream::flush() {
    AVCodecContext* codec_context = this->get_codec_context();
    avcodec_flush_buffers(codec_context);
}

enum AVMediaType MediaStream::get_media_type() const {
    return this->info.media_type;
}

double MediaStream::get_time_base() const {
    return av_q2d(this->info.stream->time_base);
}

AVCodecContext* MediaStream::get_codec_context() const {
    return this->info.codec_context;
}

MediaStream::MediaStream(StreamData& streamData) : info(streamData) { }

MediaData::~MediaData() {
    avformat_close_input(&(this->format_context));
    avformat_free_context(this->format_context);
}

//TODO: There's totally a memory leak here with the freeing of the packets
MediaStream::~MediaStream() {
    // delete this->packets;
    // delete this->info;
}

void MediaPlayer::jump_to_time(double target_time, double current_system_time) {
    if (target_time < 0.0 || target_time > this->get_duration()) {
        throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time));
    }
    const double original_time = this->playback.get_time(current_system_time);
    double final_time = original_time;

    if (this->has_video()) {
        MediaStream& video_stream = this->get_video_stream();
        video_stream.flush();
        double video_target_time = std::max(0.0, target_time - 10);
        move_packet_list_to_pts(video_stream.packets, video_target_time / video_stream.get_time_base());
        decode_until(video_stream.get_codec_context(), video_stream.packets, target_time / video_stream.get_time_base());
        final_time = video_stream.packets.get()->pts * video_stream.get_time_base();
    }

    if (this->has_audio()) {
        MediaStream& audio_stream = this->get_audio_stream();
        audio_stream.flush();
        move_packet_list_to_pts(audio_stream.packets, target_time / audio_stream.get_time_base());
        if (this->only_audio()) {
            final_time = audio_stream.packets.get()->pts * audio_stream.get_time_base();
        }
        this->cache.audio_stream.clear_and_restart_at(final_time);
    }

    double timeMoved = final_time - original_time;
    this->playback.skip(timeMoved);
    if (this->get_desync_time(current_system_time) > 0.15) {
        this->resync(current_system_time);
    }
}

bool MediaPlayer::only_audio() const {
    return this->has_audio() && !this->has_video();
}

bool MediaPlayer::only_video() const {
    return this->has_video() && !this->has_audio();
}

void clear_playhead_packet_list(PlayheadList<AVPacket*>& packets) {
    while (packets.can_step_forward()) {
        AVPacket* packet = packets.get();
        av_packet_free(&packet);
        packets.step_forward();
    }

    AVPacket* packet = packets.get();
    av_packet_free(&packet);

    packets.clear();
}

void clear_playhead_frame_list(PlayheadList<AVFrame*>& frames) {
    while (frames.can_step_forward()) {
        AVFrame* frame = frames.get();
        av_frame_free(&frame);
        frames.step_forward();
    }

    AVFrame* frame = frames.get();
    av_frame_free(&frame);

    frames.clear();
}
