
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

extern "C" {
#include <ncurses.h>
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
                if (packets.get()->pts < targetPTS) {
                    break;
                }
            } else {
                break;
            }
        } else if (packets.get()->pts < targetPTS) {
            if (packets.can_step_forward()) {
                packets.step_forward();
                if (packets.get()->pts > targetPTS) {
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

void move_packet_list_to_time_sec(PlayheadList<AVPacket*>& packets, double time) {
    double time_base = av_q2d(packets.get()->time_base);
    int64_t pts = time / time_base;
    move_packet_list_to_pts(packets, pts);
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
                if (frames.get()->pts < targetPTS) {
                    break;
                }
            } else {
                break;
            }
        } else if (frames.get()->pts < targetPTS) {
            if (frames.can_step_forward()) {
                frames.step_forward();
                if (frames.get()->pts > targetPTS) {
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

void move_frame_list_to_time_sec(PlayheadList<AVFrame*>& frames, double time) {
    double time_base = av_q2d(frames.get()->time_base);
    int64_t pts = time / time_base;
    move_frame_list_to_pts(frames, pts);
}

double MediaPlayer::get_duration() {
    return this->media_data->duration;
}

bool MediaPlayer::has_video() {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_VIDEO);
}
bool MediaPlayer::has_audio() {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_AUDIO);
}

MediaStream& MediaPlayer::get_video_stream() {
    if (this->has_video()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);
    }
    throw std::runtime_error("Cannot get video stream from MediaPlayer, no video is available to retrieve");
}

MediaStream& MediaPlayer::get_audio_stream() {
    if (this->has_audio()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_AUDIO);
    }
    throw std::runtime_error("Cannot get audio stream from MediaPlayer, no audio is available to retrieve");
}

double MediaPlayer::get_desync_time(double current_system_time) {
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

        move_packet_list_to_time_sec(audio_media_stream.packets, current_playback_time);
    }
}

void MediaPlayer::set_current_image(PixelData& data) {
    this->cache.image = PixelData(data);
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

void MediaPlayer::jump_to_time(double target_time) {
    target_time = clamp(target_time, 0.0, this->media_data->duration);
    const double original_time = this->playback.get_time(system_clock_sec());

    if (!this->media_data->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("Could not jump to time " + std::to_string( std::round(target_time * 100) / 100) + " seconds with MediaPlayer, no video stream could be found");
    }

    MediaStream& video_stream = this->media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);
    AVCodecContext* video_codec_context = video_stream.get_codec_context();
    PlayheadList<AVPacket*>& video_packets = video_stream.packets;
    double video_time_base = video_stream.get_time_base();
    const int64_t target_video_pts = target_time / video_time_base;

    if (target_time == original_time) {
        return;
     } else if (target_time < original_time) { // If the skipped time is behind the player, begins moving backward step by step until sufficiently behind the requested time
        avcodec_flush_buffers(video_codec_context);
        double testTime = std::max(0.0, target_time - 30);
        double last_time = video_packets.get()->pts * video_time_base;
        int step_direction = testTime < original_time ? -1 : 1;

        while (video_packets.can_move_index(step_direction)) {
            video_packets.move_index(step_direction);
            if (in_range(testTime, last_time, video_packets.get()->pts * video_time_base)) {
                break;
            }
            last_time = video_packets.get()->pts * video_time_base;
        }
     }

    int64_t final_pts = -1;
    std::vector<AVFrame*> decoded_frames;

    //starts decoding forward again until it gets to the targeted time
    while (final_pts < target_video_pts || video_packets.can_step_forward()) {
        video_packets.step_forward();
        try {
            decoded_frames = decode_video_packet(video_codec_context, video_packets.get());
        } catch (ascii::ffmpeg_error e) {
            if (e.is_eagain()) {
                clear_av_frame_list(decoded_frames);
                video_packets.step_forward();
                continue;
            } else {
                throw e;
            }
        }

        if (decoded_frames.size() > 0) {
            final_pts = decoded_frames[0]->pts;
        }
        clear_av_frame_list(decoded_frames);
    }

    final_pts = std::max(final_pts, video_packets.get()->pts);
    double timeMoved = (final_pts * video_time_base) - original_time;
    this->playback.skip(timeMoved);
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
