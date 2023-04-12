
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>
#include <algorithm>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>

#include "boiler.h"
#include "media.h"
#include "wtime.h"
#include "wmath.h"
#include "except.h"
#include "formatting.h"
#include "threads.h"
#include "audioresampler.h"

extern "C" {
#include "curses.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

std::string media_type_to_string(MediaType media_type) {
    switch (media_type) {
        case MediaType::VIDEO: return "video";
        case MediaType::AUDIO: return "audio";
        case MediaType::IMAGE: return "image";
    }
    throw std::runtime_error("Attempted to get Media Type string from unimplemented Media Type. Please implement all MediaType's to return a valid string. This is a developer error and bug.");
}

MediaPlayer::MediaPlayer(const char* file_name, MediaGUI starting_media_gui, MediaPlayerConfig config) {
    this->in_use = false;
    this->file_name = file_name;
    this->is_looped = false;
    this->muted = false;
    this->media_gui = starting_media_gui;
    this->audio_enabled = config.audio_enabled;

    try {
        this->format_context = open_format_context(file_name);
    } catch (std::runtime_error const& e) {
        throw std::runtime_error("Could not allocate media data of " + std::string(file_name) + " because of error while fetching file format data: " + e.what());
    } 

    std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    for (int i = 0; i < (int)media_types.size(); i++) {
        try {
            std::unique_ptr<StreamData> stream_data_ptr = std::make_unique<StreamData>(format_context, media_types[i]);
            this->media_streams.push_back(std::move(stream_data_ptr));
        } catch (std::invalid_argument const& e) {
            continue;
        }
    }

    if (avformat_context_is_static_image(this->format_context)) {
        this->media_type = MediaType::IMAGE;
    } else if (avformat_context_is_video(this->format_context)) {
        this->media_type = MediaType::VIDEO;
    } else if (avformat_context_is_audio_only(this->format_context)) {
        this->media_type = MediaType::AUDIO;
    } else {
        throw std::runtime_error("Could not find matching MediaType for file " + std::string(file_name));
    }

    if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        StreamData& audio_stream_data = this->get_media_stream(AVMEDIA_TYPE_AUDIO);
        AVCodecContext* audio_codec_context = audio_stream_data.get_codec_context();
        this->audio_buffer.init(audio_codec_context->ch_layout.nb_channels, audio_codec_context->sample_rate);
    }
}

void MediaPlayer::start() {
    this->start(0.0);
}

void MediaPlayer::start(double start_time) {
    if (this->in_use) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }
    this->in_use = true;

    if (this->media_type == MediaType::IMAGE) {
        this->frame = PixelData(this->file_name);
    } else if (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO) {
        this->playback.start(system_clock_sec());
        this->jump_to_time(start_time, system_clock_sec());
    }

    std::thread video_thread;
    bool video_thread_initialized = false;
    std::thread audio_thread;
    bool audio_thread_initialized = false;

    if (this->has_media_stream(AVMEDIA_TYPE_VIDEO) && (this->media_type == MediaType::VIDEO || this->media_type == MediaType::AUDIO)) {
        std::thread initialized_video_thread(&MediaPlayer::video_playback_thread, this);
        video_thread.swap(initialized_video_thread);
        video_thread_initialized = true;
    }

    if (this->has_media_stream(AVMEDIA_TYPE_AUDIO) && this->audio_enabled) {
        std::thread initialized_audio_thread(&MediaPlayer::audio_playback_thread, this);
        audio_thread.swap(initialized_audio_thread);
        audio_thread_initialized = true;
    }

    this->render_loop();
    if (video_thread_initialized) {
        video_thread.join();
    }
    if (audio_thread_initialized) {
        audio_thread.join();
    }

    if (this->playback.is_playing()) {
        this->playback.stop(system_clock_sec());
    }
    this->in_use = false;
}

StreamData& MediaPlayer::get_media_stream(enum AVMediaType media_type) const {
    for (int i = 0; i < (int)this->media_streams.size(); i++) {
        if (this->media_streams[i]->get_media_type() == media_type) {
            return *this->media_streams[i];
        }
    }
    throw std::runtime_error("Cannot get media stream " + std::string(av_get_media_type_string(media_type)) + " from media data, could not be found");
}

bool MediaPlayer::has_media_stream(enum AVMediaType media_type) const {
    for (int i = 0; i < (int)this->media_streams.size(); i++) {
        if (this->media_streams[i]->get_media_type() == media_type) {
            return true;
        }
    }
    return false;
}

int MediaPlayer::fetch_next(int requestedPacketCount) {
    AVPacket* reading_packet = av_packet_alloc();
    int packets_read = 0;

    while (av_read_frame(this->format_context, reading_packet) == 0) {
       
        for (int i = 0; i < (int)this->media_streams.size(); i++) {
            if (this->media_streams[i]->get_stream_index() == reading_packet->stream_index) {
                AVPacket* saved_packet = av_packet_alloc();
                av_packet_ref(saved_packet, reading_packet);
                
                this->media_streams[i]->packet_queue.push_back(saved_packet);
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
    return this->format_context->duration / AV_TIME_BASE;
}

double MediaPlayer::get_time(double current_system_time) const {
    return this->playback.get_time(current_system_time);
}

double MediaPlayer::get_desync_time(double current_system_time) const {
    if (this->has_media_stream(AVMEDIA_TYPE_AUDIO) && this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        double current_playback_time = this->get_time(current_system_time);
        double desync = std::abs(this->audio_buffer.get_time() - current_playback_time);
        return desync;
    } else { // if there is only a video or audio stream, there can never be desync
        return 0.0;
    }
}

void MediaPlayer::set_current_frame(PixelData& data) {
    this->frame = data;
}

void MediaPlayer::set_current_frame(AVFrame* frame) {
    this->frame = frame;
}


PixelData& MediaPlayer::get_current_frame() {
    return this->frame;
}

int MediaPlayer::get_nb_media_streams() const {
    return this->media_streams.size();
}

bool MediaPlayer::is_audio_enabled() const { 
    return this->audio_enabled;
}

MediaPlayer::~MediaPlayer() {
    avformat_close_input(&(this->format_context));
    avformat_free_context(this->format_context);
}


std::vector<AVFrame*> MediaPlayer::next_video_frames() {
    if (this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        StreamData& video_stream = this->get_media_stream(AVMEDIA_TYPE_VIDEO);
        std::vector<AVFrame*> decodedFrames = video_stream.decode_next();
        if (decodedFrames.size() > 0) {
            return decodedFrames;
        }

        int fetch_count = -1;
        do {
            fetch_count = video_stream.packet_queue.empty() ? this->fetch_next(10) : -1; // -1 means that no fetch request was made
            std::vector<AVFrame*> decodedFrames = video_stream.decode_next();
            if (decodedFrames.size() > 0) {
                return decodedFrames;
            } 
        } while (fetch_count != 0);

        return {}; // no video frames could sadly be found. This should only really ever happen once the file ends
    }
    throw std::runtime_error("[MediaPlayer::next_video_frames] Cannot get next video frames, as no video stream is available to decode from");
}

std::vector<AVFrame*> MediaPlayer::next_audio_frames() {
    if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        StreamData& audio_stream = this->get_media_stream(AVMEDIA_TYPE_AUDIO);
        std::vector<AVFrame*> decodedFrames = audio_stream.decode_next();
        if (decodedFrames.size() > 0) {
            return decodedFrames;
        } 

        int fetch_count = -1;
        do {
            fetch_count = audio_stream.packet_queue.empty() ? this->fetch_next(10) : -1;
            std::vector<AVFrame*> decodedFrames = audio_stream.decode_next();
            if (decodedFrames.size() > 0) {
                return decodedFrames;
            } 
        } while (fetch_count != 0);

        return {}; // no video frames could sadly be found. This should only really ever happen once the file ends
    }
    throw std::runtime_error("[MediaPlayer::next_audio_frames] Cannot get next audio frames, as no video stream is available to decode from");
}

int MediaPlayer::load_next_audio_frames(int frames) {
    if (!this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        throw std::runtime_error("[MediaPlayer::load_next_audio_frames] Cannot load next audio frames, Media Player does not have any audio stream to load data from");
    }
    if (frames < 0) {
        throw std::runtime_error("Cannot load negative frames, (got " + std::to_string(frames) + " ). ");
    }

    int written_samples = 0;
    StreamData& audio_media_stream = this->get_media_stream(AVMEDIA_TYPE_AUDIO);
    AVCodecContext* audio_codec_context = audio_media_stream.get_codec_context();
    AudioResampler audio_resampler(
        &(audio_codec_context->ch_layout), AV_SAMPLE_FMT_FLT, audio_codec_context->sample_rate,
        &(audio_codec_context->ch_layout), audio_codec_context->sample_fmt, audio_codec_context->sample_rate);

    for (int i = 0; i < frames; i++) {
        std::vector<AVFrame*> next_raw_audio_frames = this->next_audio_frames();
        std::vector<AVFrame*> audio_frames = audio_resampler.resample_audio_frames(next_raw_audio_frames);
        for (int i = 0; i < (int)audio_frames.size(); i++) {
            AVFrame* current_frame = audio_frames[i];
            float* frameData = (float*)(current_frame->data[0]);
            this->audio_buffer.write(frameData, current_frame->nb_samples);
            written_samples += current_frame->nb_samples;
        }
        clear_av_frame_list(next_raw_audio_frames);
        clear_av_frame_list(audio_frames);
    }

    return written_samples;
}

int MediaPlayer::jump_to_time(double target_time, double current_system_time) {
    if (target_time < 0.0 || target_time > this->get_duration()) {
        throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time));
    }

    const double original_time = this->get_time(current_system_time);
    int ret = avformat_seek_file(this->format_context, -1, 0.0, target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

    if (ret < 0) {
        return ret;
    }

    if (this->has_media_stream(AVMEDIA_TYPE_VIDEO)) {
        StreamData& video_media_stream = this->get_media_stream(AVMEDIA_TYPE_VIDEO);
        video_media_stream.flush();
        video_media_stream.clear_queue();

        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_video_frames();
            for (int i = 0; i < (int)frames.size(); i++) {
                if (frames[i]->pts * video_media_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);
    }

    if (this->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        StreamData& audio_media_stream = this->get_media_stream(AVMEDIA_TYPE_AUDIO);
        audio_media_stream.flush();
        audio_media_stream.clear_queue();
        this->audio_buffer.clear_and_restart_at(target_time);
        
        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_audio_frames();
            for (int i = 0; i < (int)frames.size(); i++) {
                if (frames[i]->pts * audio_media_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);
    }

    this->playback.skip(target_time - original_time); // Update the playback to account for the skipped time
    return ret;
}