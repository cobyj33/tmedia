
#include "boiler.h"
#include "decode.h"
#include "media.h"
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

const int MIN_RECOMMENDED_AUDIO_BUFFER_SIZE = 220500; // 5 seconds of audio data at 44100 Hz sample rate
const int MAX_RECOMMENDED_AUDIO_BUFFER_SIZE = 1323000; // 30 seconds of audio data at 44100 Hz sample rate
const int RECOMMENDED_AUDIO_BUFFER_SIZE = 661500; // 15 seconds of audio data at 44100 Hz sample rate
const char* DEBUG_AUDIO_SOURCE = "Audio";
const char* DEBUG_AUDIO_TYPE = "Debug";
const double MAX_AUDIO_DESYNC_TIME_SECONDS = 0.25;
const double MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS = 2.5;

typedef struct CallbackData {
    MediaPlayer* player;
    std::reference_wrapper<std::mutex> mutex_ref;

    CallbackData(MediaPlayer* player, std::reference_wrapper<std::mutex> mutex) : player(player), mutex_ref(mutex) {}
} CallbackData;

/**
 * @brief The callback called by miniaudio once the 
 * 
 * @param pDevice 
 * @param pOutput 
 * @param pInput 
 * @param frameCount 
 */
void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    CallbackData* data = (CallbackData*)(pDevice->pUserData);
    std::lock_guard<std::mutex> mutex_lock_guard(data->mutex_ref.get());
    AudioBuffer& audio_buffer = data->player->audio_buffer;

    // AudioBuffer& audio_buffer = player->audio_buffer;
   

    while (!audio_buffer.can_read(frameCount)) {
        int written_samples = data->player->load_next_audio_frames(5);
        if (written_samples == 0) {
            break;
        }
    }

    if (audio_buffer.can_read(frameCount)) {
        audio_buffer.read_into(frameCount, (float*)pOutput);
    } else {
        float* pFloatOutput = (float*)pOutput;
        for (int i = 0; i < frameCount; i++) {
            pFloatOutput[i] = 0.0;
        }
    }
}

void audio_playback_thread(MediaPlayer* player, std::mutex& alter_mutex) {
    std::unique_lock<std::mutex> mutex_lock(alter_mutex, std::defer_lock);
    int result;
    if (!player->has_audio()) {
        throw std::runtime_error("Cannot play audio playback, Audio stream could not be found");
    }

    StreamData& audio_stream_data = player->get_audio_stream_data();
    AVCodecContext* audio_codec_context = audio_stream_data.get_codec_context();
    const int nb_channels = audio_codec_context->ch_layout.nb_channels;


    AudioResampler audio_resampler(
            &(audio_codec_context->ch_layout), AV_SAMPLE_FMT_FLT, audio_codec_context->sample_rate,
            &(audio_codec_context->ch_layout), audio_codec_context->sample_fmt, audio_codec_context->sample_rate);

    player->audio_buffer.init(nb_channels, audio_codec_context->sample_rate);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format  = ma_format_f32;
    config.playback.channels = nb_channels;              
    config.sampleRate = audio_codec_context->sample_rate;           
    config.dataCallback = audioDataCallback;   

    CallbackData userData(player, std::ref(alter_mutex)); 
    config.pUserData = &userData;   

    ma_result miniaudio_log;
    ma_device audio_device;
    miniaudio_log = ma_device_init(NULL, &config, &audio_device);

    if (miniaudio_log != MA_SUCCESS) {
        throw std::runtime_error("FAILED TO INITIALIZE AUDIO DEVICE: " + std::to_string(miniaudio_log));
    }

    sleep_for_sec(audio_stream_data.get_start_time());
    
    while (player->in_use) {
        mutex_lock.lock();

        if (player->playback.is_playing() == false && ma_device_get_state(&audio_device) == ma_device_state_started) {
            mutex_lock.unlock();
            miniaudio_log = ma_device_stop(&audio_device);
            if (miniaudio_log != MA_SUCCESS) {
                ma_device_uninit(&audio_device);
                throw std::runtime_error("Failed to stop playback: Miniaudio Error " + std::to_string(miniaudio_log));
            }
            mutex_lock.lock();
        } else if (player->playback.is_playing() && ma_device_get_state(&audio_device) == ma_device_state_stopped) {
            mutex_lock.unlock();
            miniaudio_log = ma_device_start(&audio_device);
            if (miniaudio_log != MA_SUCCESS) {
                ma_device_uninit(&audio_device);
                throw std::runtime_error("Failed to start playback: Miniaudio Error " + std::to_string(miniaudio_log));
            }
            mutex_lock.lock();
        }

        double current_system_time = system_clock_sec();
        if (player->get_desync_time(current_system_time) > MAX_AUDIO_DESYNC_TIME_SECONDS) {
            double target_resync_time = player->get_time(current_system_time);

            // if (player->audio_buffer.is_time_in_bounds(target_resync_time)) {
            //     player->audio_buffer.set_time(target_resync_time);
            // } else if (target_resync_time > player->audio_buffer.get_time() && target_resync_time < player->audio_buffer.get_time() + MAX_AUDIO_CATCHUP_DECODE_TIME_SECONDS) {

            //     int writtenSamples = 0;
            //     do {
            //         writtenSamples = player->load_next_audio_frames(20);
            //     } while (writtenSamples != 0 && !player->audio_buffer.is_time_in_bounds(target_resync_time));

            //     if (player->audio_buffer.is_time_in_bounds(target_resync_time)) {
            //         player->audio_buffer.set_time(target_resync_time);
            //     } else {
            //         player->jump_to_time(target_resync_time, current_system_time);
            //     }

            // } else {
            //     player->jump_to_time(target_resync_time, current_system_time);
            // }

            // if (player->audio_buffer.is_time_in_bounds(target_resync_time)) {
            //     player->audio_buffer.set_time(target_resync_time);
            // } else {
            //     player->jump_to_time(target_resync_time, current_system_time);
            // }

            player->jump_to_time(target_resync_time, current_system_time);
            
        }

        if (!player->audio_buffer.can_read(RECOMMENDED_AUDIO_BUFFER_SIZE)) {
            player->load_next_audio_frames(10);
        }

        audio_device.sampleRate = audio_codec_context->sample_rate * player->playback.get_speed();
        
        mutex_lock.unlock();
        sleep_for_ms(5);
    }

}

