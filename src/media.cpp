
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>

#include "boiler.h"
#include "playheadlist.hpp"
#include "media.h"
#include "wtime.h"
#include "wmath.h"
#include "except.h"
#include "formatting.h"
#include "threads.h"

extern "C" {
#include "ncurses.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

void MediaPlayer::start(MediaGUI media_gui, double start_time) {
    if (this->in_use) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    this->in_use = true;
    this->playback.start(system_clock_sec());
    this->jump_to_time(start_time, system_clock_sec());


    std::mutex alter_mutex;
    std::thread video_thread(video_playback_thread, this, std::ref(alter_mutex));
    std::thread audio_thread(audio_playback_thread, this, std::ref(alter_mutex));
    render_loop(this, std::ref(alter_mutex), media_gui);

    video_thread.join();
    audio_thread.join();

    this->playback.stop(system_clock_sec());
    this->in_use = false;
}

StreamData& MediaData::get_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->nb_streams; i++) {
        if (this->media_streams[i]->get_media_type() == media_type) {
            return *this->media_streams[i];
        }
    }
    throw std::runtime_error("Cannot get media stream " + std::string(av_get_media_type_string(media_type)) + " from media data, could not be found");
}

bool MediaData::has_media_stream(enum AVMediaType media_type) {
    try {
        this->get_media_stream(media_type);
        return true;
    } catch (std::runtime_error not_found_error) {
        return false;
    }
}

int MediaData::fetch_next(int requestedPacketCount) {
    AVPacket* reading_packet = av_packet_alloc();
    int packets_read = 0;

    while (av_read_frame(this->format_context, reading_packet) == 0) {
       
        for (int i = 0; i < this->nb_streams; i++) {
            if (this->media_streams[i]->get_stream_index() == reading_packet->stream_index) {
                AVPacket* saved_packet = av_packet_alloc();
                av_packet_ref(saved_packet, reading_packet);
                
                this->media_streams[i]->packets.push_back(saved_packet);
                packets_read++;
                break;
            }
        }

        av_packet_unref(reading_packet);

         if (packets_read >= requestedPacketCount) {
            av_packet_free(&reading_packet);
            return packets_read;
        }
    }

    //reached EOF

    av_packet_free(&reading_packet);
    return packets_read;
}

double MediaPlayer::get_duration() const {
    return this->media_data->duration;
}

double MediaPlayer::get_time(double current_system_time) const {
    return this->playback.get_time(current_system_time);
}

bool MediaPlayer::has_video() const {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_VIDEO);
}
bool MediaPlayer::has_audio() const {
    return this->media_data->has_media_stream(AVMEDIA_TYPE_AUDIO);
}

StreamData& MediaPlayer::get_video_stream() const {
    if (this->has_video()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_VIDEO);
    }
    throw std::runtime_error("Cannot get video stream from MediaPlayer, no video is available to retrieve");
}

StreamData& MediaPlayer::get_audio_stream() const {
    if (this->has_audio()) {
        return this->media_data->get_media_stream(AVMEDIA_TYPE_AUDIO);
    }
    throw std::runtime_error("Cannot get audio stream from MediaPlayer, no audio is available to retrieve");
}

double MediaPlayer::get_desync_time(double current_system_time) const {
    if (this->has_audio() && this->has_video()) {
        double current_playback_time = this->get_time(current_system_time);
        double desync = std::abs(this->cache.audio_stream.get_time() - current_playback_time);
        return desync;
    } else { // if there is only a video or audio stream, there can never be desync
        return 0.0;
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
        std::unique_ptr<StreamData> media_stream_ptr = std::make_unique<StreamData>( (*this->stream_datas)[i] );
        this->media_streams.push_back(std::move(media_stream_ptr));
    }

    this->duration = (double)this->format_context->duration / AV_TIME_BASE;
}

MediaData::~MediaData() {
    avformat_close_input(&(this->format_context));
    avformat_free_context(this->format_context);
}


std::vector<AVFrame*> MediaPlayer::next_video_frames() {
    if (this->has_video()) {
        StreamData& video_stream = this->get_video_stream();
        std::vector<AVFrame*> decodedFrames = video_stream.decode_next();
        if (decodedFrames.size() > 0) {
            return decodedFrames;
        }

        
        int fetch_count = 0;
        do {
            fetch_count = this->media_data->fetch_next(10);
            std::vector<AVFrame*> decodedFrames = video_stream.decode_next();
            if (decodedFrames.size() > 0) {
                return decodedFrames;
            } 
        } while (fetch_count > 0);

        return {}; // no video frames could sadly be found. This should only really ever happen once the file ends
    }
    throw std::runtime_error("[MediaPlayer::next_video_frames] Cannot get next video frames, as no video stream is available to decode from");
}

std::vector<AVFrame*> MediaPlayer::next_audio_frames() {
    if (this->has_audio()) {
        StreamData& audio_stream = this->get_audio_stream();
        std::vector<AVFrame*> decodedFrames = audio_stream.decode_next();
        if (decodedFrames.size() > 0) {
            return decodedFrames;
        } 

        
        int fetch_count = 0;
        do {
            fetch_count = this->media_data->fetch_next(10);
            std::vector<AVFrame*> decodedFrames = audio_stream.decode_next();
            if (decodedFrames.size() > 0) {
                return decodedFrames;
            } 
        } while (fetch_count > 0);


        return {}; // no video frames could sadly be found. This should only really ever happen once the file ends
    }
    throw std::runtime_error("[MediaPlayer::next_audio_frames] Cannot get next audio frames, as no video stream is available to decode from");
}

void MediaPlayer::load_next_audio_frames(int frames) {
    if (!this->has_audio()) {
        throw std::runtime_error("[MediaPlayer::load_next_audio_frames] Cannot load next audio frames, Media Player does not have any audio data to load");
    }
    if (frames < 0) {
        throw std::runtime_error("Cannot load negative frames, (got " + std::to_string(frames) + " ). ");
    }

    StreamData& audio_media_stream = this->get_audio_stream();
    AVCodecContext* audio_codec_context = audio_media_stream.get_codec_context();
    AudioResampler audio_resampler(
        &(audio_codec_context->ch_layout), AV_SAMPLE_FMT_FLT, audio_codec_context->sample_rate,
        &(audio_codec_context->ch_layout), audio_codec_context->sample_fmt, audio_codec_context->sample_rate);

    for (int i = 0; i < frames; i++) {
        std::vector<AVFrame*> next_raw_audio_frames = this->next_audio_frames();
        std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(next_raw_audio_frames);
        for (int i = 0; i < audio_frames.size(); i++) {
            AVFrame* current_frame = audio_frames[i];
            float* frameData = (float*)(current_frame->data[0]);
            this->cache.audio_stream.write(frameData, current_frame->nb_samples);
        }
        clear_av_frame_list(next_raw_audio_frames);
        clear_av_frame_list(audio_frames);
    }
}

void MediaPlayer::jump_to_time(double target_time, double current_system_time) {
    if (target_time < 0.0 || target_time > this->get_duration()) {
        throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time));
    }

    const double EXACT_SEEK_TIME_START_THRESHOLD = 5.0;
    const double SEEK_TIME_OFFSET = 1.0;
    double seek_time = target_time > EXACT_SEEK_TIME_START_THRESHOLD ? target_time - SEEK_TIME_OFFSET : target_time;

    //seek_time <= target_time is always true

    const double original_time = this->get_time(current_system_time);
    int ret = avformat_seek_file(this->media_data->format_context, -1, 0.0, seek_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

    if (ret < 0) {
        throw std::runtime_error("[MediaPlayer::jump_to_time] Failed to seek file");
    }

    if (this->has_video()) {
        StreamData& video_stream = this->get_video_stream();
        video_stream.flush();
        clear_playhead_packet_list(video_stream.packets);
    }

    if (this->has_audio()) {
        StreamData& audio_stream = this->get_audio_stream();
        audio_stream.flush();
        clear_playhead_packet_list(audio_stream.packets);
    }

    this->media_data->fetch_next(1000);

    if (this->has_video()) {
        StreamData& video_stream = this->get_video_stream();
        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_video_frames();
            for (int i = 0; i < frames.size(); i++) {
                if (frames[i]->pts * video_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);
    }

    if (this->has_audio()) {
        StreamData& audio_stream = this->get_audio_stream();
        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_audio_frames();
            for (int i = 0; i < frames.size(); i++) {
                if (frames[i]->pts * audio_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);

        this->cache.audio_stream.clear_and_restart_at(target_time);
        this->load_next_audio_frames(40);
    }


    this->playback.skip(target_time - original_time); // Update the playback to account for the skipped time
}

bool MediaPlayer::only_audio() const {
    return this->has_audio() && !this->has_video();
}

bool MediaPlayer::only_video() const {
    return this->has_video() && !this->has_audio();
}

void clear_playhead_packet_list(PlayheadList<AVPacket*>& packets) {
    if (packets.is_empty()) {
        return;
    }

    packets.set_index(0);
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
    if (frames.is_empty()) {
        return;
    }

    frames.set_index(0);
    while (frames.can_step_forward()) {
        AVFrame* frame = frames.get();
        av_frame_free(&frame);
        frames.step_forward();
    }

    AVFrame* frame = frames.get();
    av_frame_free(&frame);

    frames.clear();
}

void clear_behind_packet_list(PlayheadList<AVPacket*>& packets) {
    if (packets.is_empty()) {
        return;
    }

    int index = packets.get_index();
    while (packets.can_step_backward()) {
        packets.step_backward();
        AVPacket* packet = packets.get();
        av_packet_free(&packet);
    }

    packets.set_index(index);
    packets.clear_behind();
}