
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
    double timeBase = av_q2d(packets.get()->time_base);
    int64_t pts = time / timeBase;
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
    double timeBase = av_q2d(frames.get()->time_base);
    int64_t pts = time / timeBase;
    move_frame_list_to_pts(frames, pts);
}

double MediaPlayer::get_duration() {
    return this->mediaData->duration;
}

bool MediaPlayer::has_video() {
    return this->mediaData->has_media_stream(AVMEDIA_TYPE_VIDEO);
}
bool MediaPlayer::has_audio() {
    return this->mediaData->has_media_stream(AVMEDIA_TYPE_AUDIO);
}

MediaStream& MediaPlayer::get_video_stream() {
    if (this->has_video()) {
        return this->mediaData->get_media_stream(AVMEDIA_TYPE_VIDEO);
    }
    throw std::runtime_error("Cannot get video stream from MediaPlayer, no video is available to retrieve");
}

MediaStream& MediaPlayer::get_audio_stream() {
    if (this->has_audio()) {
        return this->mediaData->get_media_stream(AVMEDIA_TYPE_AUDIO);
    }
    throw std::runtime_error("Cannot get audio stream from MediaPlayer, no audio is available to retrieve");
}

double MediaPlayer::get_desync_time(double current_system_time) {
    if (this->has_audio() && this->has_video()) {
        double current_playback_time = this->playback.get_time(current_system_time);
        double desync = std::abs(this->displayCache.audio_stream.get_time() - current_playback_time);
        return desync;
    } else {
        return 0.0;
    }
}

void MediaPlayer::resync(double current_system_time) {

    if (this->has_audio()) {
        AudioStream& audioStream = this->displayCache.audio_stream;
        MediaStream& audio_media_stream = this->get_audio_stream();


        double current_playback_time = this->playback.get_time(current_system_time);
        if (audioStream.is_time_in_bounds(current_playback_time)) {
            audioStream.set_time(current_playback_time);
        } else {
            audioStream.clear_and_restart_at(current_playback_time);
        }

        move_packet_list_to_time_sec(audio_media_stream.packets, current_playback_time);
    }
}

void MediaPlayer::set_current_image(PixelData& data) {
    this->displayCache.image = PixelData(data);
}

PixelData& MediaPlayer::get_current_image() {
    return this->displayCache.image;
}

MediaDisplaySettings::MediaDisplaySettings() {
    this->use_colors = false;
}

MediaData::MediaData(const char* fileName) {
    int result;

    try {
        this->formatContext = open_format_context(fileName);
    } catch (std::runtime_error e) {
        throw std::runtime_error("Could not allocate media data of " + std::string(fileName) + " because of error while fetching file format data: " + e.what());
    } 

    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    this->stream_datas = std::make_unique<StreamDataGroup>(this->formatContext, mediaTypes, 2);

    this->allPacketsRead = false;
    this->nb_streams = this->stream_datas->get_nb_streams();

    for (int i = 0; i < this->nb_streams; i++) {
        std::unique_ptr<MediaStream> media_stream_ptr = std::make_unique<MediaStream>( (*this->stream_datas)[i] );
        this->media_streams.push_back(std::move(media_stream_ptr));
    }

    this->duration = (double)this->formatContext->duration / AV_TIME_BASE;
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
    return this->info.mediaType;
}

double MediaStream::get_time_base() const {
    return av_q2d(this->info.stream->time_base);
}

AVCodecContext* MediaStream::get_codec_context() const {
    return this->info.codecContext;
}

MediaStream::MediaStream(StreamData& streamData) : info(streamData) { }

MediaData::~MediaData() {
    avformat_close_input(&(this->formatContext));
    avformat_free_context(this->formatContext);
}

//TODO: There's totally a memory leak here with the freeing of the packets
MediaStream::~MediaStream() {
    // delete this->packets;
    // delete this->info;
}

void MediaPlayer::jump_to_time(double targetTime) {
    targetTime = clamp(targetTime, 0.0, this->mediaData->duration);
    const double originalTime = this->playback.get_time(system_clock_sec());

    if (!this->mediaData->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("Could not jump to time " + std::to_string( std::round(targetTime * 100) / 100) + " seconds with MediaPlayer, no video stream could be found");
    }

    MediaStream& video_stream = this->mediaData->get_media_stream(AVMEDIA_TYPE_VIDEO);
    AVCodecContext* videoCodecContext = video_stream.get_codec_context();
    PlayheadList<AVPacket*>& videoPackets = video_stream.packets;
    double videoTimeBase = video_stream.get_time_base();
    const int64_t targetVideoPTS = targetTime / videoTimeBase;

    if (targetTime == originalTime) {
        return;
     } else if (targetTime < originalTime) { // If the skipped time is behind the player, begins moving backward step by step until sufficiently behind the requested time
        avcodec_flush_buffers(videoCodecContext);
        double testTime = std::max(0.0, targetTime - 30);
        double last_time = videoPackets.get()->pts * videoTimeBase;
        int step_direction = testTime < originalTime ? -1 : 1;

        while (videoPackets.can_move_index(step_direction)) {
            videoPackets.move_index(step_direction);
            if (in_range(testTime, last_time, videoPackets.get()->pts * videoTimeBase)) {
                break;
            }
            last_time = videoPackets.get()->pts * videoTimeBase;
        }
     }

    int64_t finalPTS = -1;
    std::vector<AVFrame*> decodedFrames;

    //starts decoding forward again until it gets to the targeted time
    while (finalPTS < targetVideoPTS || videoPackets.can_step_forward()) {
        videoPackets.step_forward();
        try {
            decodedFrames = decode_video_packet(videoCodecContext, videoPackets.get());
        } catch (ascii::ffmpeg_error e) {
            if (e.is_eagain()) {
                clear_av_frame_list(decodedFrames);
                videoPackets.step_forward();
                continue;
            } else {
                throw e;
            }
        }

        if (decodedFrames.size() > 0) {
            finalPTS = decodedFrames[0]->pts;
        }
        clear_av_frame_list(decodedFrames);
    }

    finalPTS = std::max(finalPTS, videoPackets.get()->pts);
    double timeMoved = (finalPTS * videoTimeBase) - originalTime;
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
