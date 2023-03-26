#ifndef ASCII_VIDEO_DECODE
#define ASCII_VIDEO_DECODE

#include <vector>
#include <deque>
#include "audioresampler.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


// void free_frame_list(std::vector<AVFrame*> frame_list, int count);
/**
 * @brief clears the AVFrame vector of all it's contents **AND FREES ALL AVFRAME's CONTAINED WITHIN**
 * @param frame_list 
 */
void clear_av_frame_list(std::vector<AVFrame*>& frame_list);

std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, AVPacket* video_packet);
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, AVPacket* audio_packet);
std::vector<AVFrame*> decode_video_packet_queue(AVCodecContext* video_codec_context, std::deque<AVPacket*>& video_packet_queue);
std::vector<AVFrame*> decode_audio_packet_queue(AVCodecContext* audio_codec_context, std::deque<AVPacket*>& audio_packet_queue);
void decode_video_packet_queue_until(AVCodecContext* video_codec_context, std::deque<AVPacket*>& video_packet_queue, int64_t target_pts); // mostly meant for media seeking

#endif
