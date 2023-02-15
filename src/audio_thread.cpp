
#include "boiler.h"
#include "debug.h"
#include "decode.h"
#include "media.h"
#include "playheadlist.hpp"
#include "wtime.h"
#include "audio.h"
#include "wmath.h"
#include "except.h"
#include "pixeldata.h"
#include "threads.h"
#include "audioresampler.h"

#include <cstdlib>
#include <memory>
#include <mutex>


extern "C" {
#include <miniaudio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

const int AUDIO_BUFFER_SIZE = 8192;
const int MAX_AUDIO_BUFFER_SIZE = 524288;
const char* DEBUG_AUDIO_SOURCE = "Audio";
const char* DEBUG_AUDIO_TYPE = "Debug";

typedef struct CallbackData {
    MediaPlayer* player;
    std::reference_wrapper<std::mutex> mutex;
    CallbackData(MediaPlayer* player, std::reference_wrapper<std::mutex> mutex) : player(player), mutex(mutex) {}
} CallbackData;

void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    CallbackData* data = (CallbackData*)(pDevice->pUserData);
    std::lock_guard<std::mutex> mutex_lock_guard(data->mutex.get());
    AudioStream& audioStream = data->player->displayCache.audio_stream;

    if (audioStream.can_read(frameCount)) {
        audioStream.read_into(frameCount, (float*)pOutput);
    }
}

void audio_playback_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    std::unique_lock<std::mutex> mutex_lock(alter_mutex, std::defer_lock);
    int result;
    if (!player->timeline->mediaData->has_media_stream(AVMEDIA_TYPE_AUDIO)) {
        throw std::runtime_error("Cannot play audio playback, Audio stream could not be found");
    }

    MediaStream& audio_media_stream = player->timeline->mediaData->get_media_stream(AVMEDIA_TYPE_AUDIO);
    AVCodecContext* audioCodecContext = audio_media_stream.get_codec_context();
    const int nb_channels = audioCodecContext->ch_layout.nb_channels;


    AudioResampler audioResampler(
            &(audioCodecContext->ch_layout), AV_SAMPLE_FMT_FLT, audioCodecContext->sample_rate,
            &(audioCodecContext->ch_layout), audioCodecContext->sample_fmt, audioCodecContext->sample_rate);

    player->displayCache.audio_stream.init(nb_channels, audioCodecContext->sample_rate);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = nb_channels;              
    config.sampleRate = audioCodecContext->sample_rate;           
    config.dataCallback = audioDataCallback;   

    CallbackData userData(player, std::ref(alter_mutex)); 
    config.pUserData = &userData;   

    ma_result miniAudioLog;
    ma_device audioDevice;
    miniAudioLog = ma_device_init(NULL, &config, &audioDevice);

    if (miniAudioLog != MA_SUCCESS) {
        throw std::runtime_error("FAILED TO INITIALIZE AUDIO DEVICE: " + std::to_string(miniAudioLog));
    }

    Playback& playback = player->timeline->playback;
    sleep_for_sec(audio_media_stream.get_start_time());
    
    while (player->inUse) {
        mutex_lock.lock();

        if (playback.is_playing() == false && ma_device_get_state(&audioDevice) == ma_device_state_started) {
            mutex_lock.unlock();
            miniAudioLog = ma_device_stop(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                ma_device_uninit(&audioDevice);
                throw std::runtime_error("Failed to stop playback: Miniaudio Error " + std::to_string(miniAudioLog));
            }
            mutex_lock.lock();
        } else if (playback.is_playing() && ma_device_get_state(&audioDevice) == ma_device_state_stopped) {
            mutex_lock.unlock();
            miniAudioLog = ma_device_start(&audioDevice);
            if (miniAudioLog != MA_SUCCESS) {
                ma_device_uninit(&audioDevice);
                throw std::runtime_error("Failed to start playback: Miniaudio Error " + std::to_string(miniAudioLog));
            }
            mutex_lock.lock();
        }

        AudioStream& audioStream = player->displayCache.audio_stream;
        std::vector<AVFrame*> audioFrames = get_final_audio_frames(audioCodecContext, audioResampler, audio_media_stream.packets);
        for (int i = 0; i < audioFrames.size(); i++) {
            AVFrame* current_frame = audioFrames[i];
            float* frameData = (float*)(current_frame->data[0]);
            audioStream.write(frameData, current_frame->nb_samples);
        }
        clear_av_frame_list(audioFrames);

        audio_media_stream.packets.try_step_forward();

        audioDevice.sampleRate = audioCodecContext->sample_rate * playback.get_speed();

        const double MAX_AUDIO_DESYNC_TIME_SECONDS = 0.15;
        if (player->get_desync_time(system_clock_sec()) > MAX_AUDIO_DESYNC_TIME_SECONDS) {
            player->resync(system_clock_sec());
        }

        mutex_lock.unlock();
        sleep_for_ms(1);
    }

}

