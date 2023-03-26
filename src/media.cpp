
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

extern "C" {
#include "ncurses.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

void MediaPlayer::start(double start_time) {
    if (this->in_use) {
        throw std::runtime_error("CANNOT USE MEDIA PLAYER THAT IS ALREADY IN USE");
    }

    this->in_use = true;
    this->playback.start(system_clock_sec());
    this->jump_to_time(start_time, system_clock_sec());


    std::mutex alter_mutex;

    std::thread video_thread(video_playback_thread, this, std::ref(alter_mutex));
    bool audio_thread_initialized = false;
    std::thread audio_thread;

    if (this->has_audio()) {
        std::thread initialized_audio_thread(audio_playback_thread, this, std::ref(alter_mutex));
        audio_thread.swap(initialized_audio_thread);
        audio_thread_initialized = true;
    }

    render_loop(this, std::ref(alter_mutex));

    video_thread.join();
    if (audio_thread_initialized) {
        audio_thread.join();
    }

    this->playback.stop(system_clock_sec());
    this->in_use = false;
}

StreamData& MediaPlayer::get_media_stream(enum AVMediaType media_type) const {
    for (int i = 0; i < this->media_streams.size(); i++) {
        if (this->media_streams[i]->get_media_type() == media_type) {
            return *this->media_streams[i];
        }
    }
    throw std::runtime_error("Cannot get media stream " + std::string(av_get_media_type_string(media_type)) + " from media data, could not be found");
}

bool MediaPlayer::has_media_stream(enum AVMediaType media_type) const {
    try {
        this->get_media_stream(media_type);
        return true;
    } catch (std::runtime_error not_found_error) {
        return false;
    }
}

int MediaPlayer::fetch_next(int requestedPacketCount) {
    AVPacket* reading_packet = av_packet_alloc();
    int packets_read = 0;

    while (av_read_frame(this->format_context, reading_packet) == 0) {
       
        for (int i = 0; i < this->media_streams.size(); i++) {
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

bool MediaPlayer::has_video() const {
    return this->has_media_stream(AVMEDIA_TYPE_VIDEO);
}
bool MediaPlayer::has_audio() const {
    return this->has_media_stream(AVMEDIA_TYPE_AUDIO);
}

StreamData& MediaPlayer::get_video_stream_data() const {
    if (this->has_video()) {
        return this->get_media_stream(AVMEDIA_TYPE_VIDEO);
    }
    throw std::runtime_error("Cannot get video stream from MediaPlayer, no video is available to retrieve");
}

StreamData& MediaPlayer::get_audio_stream_data() const {
    if (this->has_audio()) {
        return this->get_media_stream(AVMEDIA_TYPE_AUDIO);
    }
    throw std::runtime_error("Cannot get audio stream from MediaPlayer, no audio is available to retrieve");
}

double MediaPlayer::get_desync_time(double current_system_time) const {
    if (this->has_audio() && this->has_video()) {
        double current_playback_time = this->get_time(current_system_time);
        double desync = std::abs(this->audio_buffer.get_time() - current_playback_time);
        return desync;
    } else { // if there is only a video or audio stream, there can never be desync
        return 0.0;
    }
}

void MediaPlayer::set_current_frame(PixelData& data) {
    this->frame = PixelData(data);
}

void MediaPlayer::set_current_frame(AVFrame* frame) {
    this->frame = PixelData(frame);
}


PixelData& MediaPlayer::get_current_frame() {
    return this->frame;
}

MediaPlayer::MediaPlayer(const char* file_name) {
    this->in_use = false;
    this->file_name = file_name;
    this->is_looped = false;
    this->media_gui = MediaGUI();

    try {
        this->format_context = open_format_context(file_name);
    } catch (std::runtime_error e) {
        throw std::runtime_error("Could not allocate media data of " + std::string(file_name) + " because of error while fetching file format data: " + e.what());
    } 

    std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    for (int i = 0; i < media_types.size(); i++) {
        try {
            std::unique_ptr<StreamData> stream_data_ptr = std::make_unique<StreamData>(format_context, media_types[i]);
            this->media_streams.push_back(std::move(stream_data_ptr));
        } catch (std::invalid_argument e) {
            continue;
        }
    }

}

MediaPlayer::MediaPlayer(const char* file_name, MediaGUI starting_media_gui) {
    this->in_use = false;
    this->file_name = file_name;
    this->is_looped = false;
    this->media_gui = starting_media_gui;

    try {
        this->format_context = open_format_context(file_name);
    } catch (std::runtime_error e) {
        throw std::runtime_error("Could not allocate media data of " + std::string(file_name) + " because of error while fetching file format data: " + e.what());
    } 

    std::vector<enum AVMediaType> media_types = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };
    for (int i = 0; i < media_types.size(); i++) {
        try {
            std::unique_ptr<StreamData> stream_data_ptr = std::make_unique<StreamData>(format_context, media_types[i]);
            this->media_streams.push_back(std::move(stream_data_ptr));
        } catch (std::invalid_argument e) {
            continue;
        }
    }
}

int MediaPlayer::get_nb_media_streams() const {
    return this->media_streams.size();
}

MediaPlayer::~MediaPlayer() {
    avformat_close_input(&(this->format_context));
    avformat_free_context(this->format_context);
}


std::vector<AVFrame*> MediaPlayer::next_video_frames() {
    if (this->has_video()) {
        StreamData& video_stream = this->get_video_stream_data();
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
    if (this->has_audio()) {
        StreamData& audio_stream = this->get_audio_stream_data();
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
    if (!this->has_audio()) {
        throw std::runtime_error("[MediaPlayer::load_next_audio_frames] Cannot load next audio frames, Media Player does not have any audio data to load");
    }
    if (frames < 0) {
        throw std::runtime_error("Cannot load negative frames, (got " + std::to_string(frames) + " ). ");
    }

    int written_samples = 0;
    StreamData& audio_media_stream = this->get_audio_stream_data();
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
            this->audio_buffer.write(frameData, current_frame->nb_samples);
            written_samples += current_frame->nb_samples;
        }
        clear_av_frame_list(next_raw_audio_frames);
        clear_av_frame_list(audio_frames);
    }

    return written_samples;
}

void MediaPlayer::jump_to_time(double target_time, double current_system_time) {
    if (target_time < 0.0 || target_time > this->get_duration()) {
        throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time));
    }

    const double original_time = this->get_time(current_system_time);
    int ret = avformat_seek_file(this->format_context, -1, 0.0, target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

    if (ret < 0) {
        throw std::runtime_error("[MediaPlayer::jump_to_time] Failed to seek file");
    }

    if (this->has_video()) {
        StreamData& video_media_stream = this->get_video_stream_data();
        video_media_stream.flush();
        video_media_stream.clear_queue();

        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_video_frames();
            for (int i = 0; i < frames.size(); i++) {
                if (frames[i]->pts * video_media_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);
    }

    if (this->has_audio()) {
        StreamData& audio_media_stream = this->get_audio_stream_data();
        audio_media_stream.flush();
        audio_media_stream.clear_queue();
        this->audio_buffer.clear_and_restart_at(target_time);
        
        std::vector<AVFrame*> frames;
        bool passed_target_time = false;
        do {
            clear_av_frame_list(frames);
            frames = this->next_audio_frames();
            for (int i = 0; i < frames.size(); i++) {
                if (frames[i]->pts * audio_media_stream.get_time_base() >= target_time) {
                    passed_target_time = true;
                    break;
                }
            }
        } while (!passed_target_time && frames.size() > 0);
        clear_av_frame_list(frames);
    }

    this->playback.skip(target_time - original_time); // Update the playback to account for the skipped time
}

bool MediaPlayer::only_audio() const {
    return this->has_audio() && !this->has_video();
}

bool MediaPlayer::only_video() const {
    return this->has_video() && !this->has_audio();
}