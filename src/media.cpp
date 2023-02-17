
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
    while (!packets.at_edge()) {

        if (packets.get()->pts == targetPTS) {
            break;
        } else if  (std::min(packets.get()->pts, lastPTS) <= targetPTS && std::max(packets.get()->pts, lastPTS) >= targetPTS) {
            break;
        } else if (packets.get()->pts > targetPTS) {
            packets.step_backward();
            if (packets.get()->pts < targetPTS) {
                break;
            }
        } else if (packets.get()->pts < targetPTS) {
            packets.step_forward();
            if (packets.get()->pts > targetPTS) {
                packets.step_backward();
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
    while (!frames.at_edge() && !(frames.get()->pts == targetPTS)  ) {
        if (std::min(frames.get()->pts, lastPTS) <= targetPTS && std::max(frames.get()->pts, lastPTS) >= targetPTS) {
            break;
        } else if (frames.get()->pts > targetPTS) {
            frames.step_backward();
            if (frames.get()->pts < targetPTS) {
                break;
            }
        } else if (frames.get()->pts < targetPTS) {
            frames.step_forward();
            if (frames.get()->pts > targetPTS) {
                frames.step_backward();
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



double MediaPlayer::get_desync_time(double current_system_time) {
    AudioStream& audioStream = this->displayCache.audio_stream;
    Playback& playback = this->timeline->playback;

    double current_playback_time = playback.get_time(current_system_time);
    double desync = std::abs(audioStream.get_time() - current_playback_time);
    return desync;
}

void MediaPlayer::resync(double current_system_time) {

    if (this->timeline->mediaData->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        Playback& playback = this->timeline->playback;
        AudioStream& audioStream = this->displayCache.audio_stream;
        MediaStream& audio_media_stream = this->timeline->mediaData->get_media_stream(AVMEDIA_TYPE_AUDIO);


        double current_playback_time = playback.get_time(current_system_time);
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

MediaTimeline::MediaTimeline(const char* fileName) {
    this->mediaData = std::make_unique<MediaData>(fileName);
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

void MediaTimeline::jump_to_time(double targetTime) {
    targetTime = std::max(targetTime, 0.0);
    Playback& playback = this->playback;
    const double originalTime = this->playback.get_time(system_clock_sec());

    if (!this->mediaData->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        throw std::runtime_error("Could not jump to time " + std::to_string( std::round(targetTime * 100) / 100) + " seconds with MediaTimeline, no video stream could be found");
    }

    MediaStream& video_stream = this->mediaData->get_media_stream(AVMEDIA_TYPE_VIDEO);
    AVCodecContext* videoCodecContext = video_stream.get_codec_context();
    PlayheadList<AVPacket*>& videoPackets = video_stream.packets;
    double videoTimeBase = video_stream.get_time_base();
    const int64_t targetVideoPTS = targetTime / videoTimeBase;
    AVPacket* packet_get;

    if (targetTime == originalTime) {
        return;
     } else if (targetTime < originalTime || targetTime > originalTime + 60) { // If the skipped time is behind the player, begins moving backward step by step until sufficiently behind the requested time
        avcodec_flush_buffers(videoCodecContext);
        double testTime = std::max(0.0, targetTime - 30);
        double last_time = videoPackets.get()->pts * videoTimeBase;
        int step_direction = signum(testTime - originalTime);

        while (videoPackets.can_move_index(step_direction)) {
            videoPackets.move_index(step_direction);
            if (in_range(testTime, last_time, videoPackets.get()->pts * videoTimeBase)) {
                break;
            }
            last_time = packet_get->pts * videoTimeBase;
        }
     }

    int64_t finalPTS = -1;
    std::vector<AVFrame*> decodedFrames;

    //starts decoding forward again until it gets to the targeted time
    while (finalPTS < targetVideoPTS || videoPackets.can_step_forward()) {
        try {
            decodedFrames = decode_video_packet(videoCodecContext, videoPackets.get());
            videoPackets.step_forward();
        } catch (ascii::ffmpeg_error e) {
            if (e.is_eagain()) {
                clear_av_frame_list(decodedFrames);
                videoPackets.step_forward();
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
    playback.skip(timeMoved);
}