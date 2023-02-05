#ifndef ASCII_VIDEO_DECODE
#define ASCII_VIDEO_DECODE

#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

typedef std::vector<AVFrame*> (*decoder_function)(AVCodecContext*, AVPacket*, int*, int*);
decoder_function get_stream_decoder(enum AVMediaType mediaType);

// void free_frame_list(std::vector<AVFrame*> frameList, int count);
/**
 * @brief clears the AVFrame vector of all it's contents **AND FREES ALL AVFRAME's CONTAINED WITHIN**
 * @param frameList 
 */
void clear_av_frame_list(std::vector<AVFrame*>& frameList);
std::vector<AVFrame*> decode_video_packet(AVCodecContext* videoCodecContext, AVPacket* videoPacket);
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket);
#endif
