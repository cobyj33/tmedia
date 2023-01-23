#ifndef ASCII_VIDEO_AUDIO_IMPLEMENTATION
#define ASCII_VIDEO_AUDIO_IMPLEMENTATION

#define MAX_AUDIO_ASYNC_TIME_SECONDS 0.15

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}

float** alloc_samples(int linesize[8], int nb_channels, int nb_samples);
float** copy_samples(float** src, int linesize[8], int nb_channels, int nb_samples);
void free_samples(float** samples);

float** alterAudioSampleLength(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** stretchAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);
float** shrinkAudioSamples(float** originalSamples, int linesize[8], int nb_samples, int nb_channels, int target_nb_samples);

uint8_t float_sample_to_uint8(float num);
float uint8_sample_to_float(uint8_t sample);
#endif
