#ifndef ASCII_VIDEO_DECODE
#define ASCII_VIDEO_DECODE

#include <vector>
#include "playheadlist.hpp"
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
std::vector<AVFrame*> decode_video_packet(AVCodecContext* video_codec_context, PlayheadList<AVPacket*>& videoPacketList);
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audio_codec_context, PlayheadList<AVPacket*>& audio_packet_list);

std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audio_codec_context, AudioResampler& audio_resampler, AVPacket* packet);
std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audio_codec_context, AudioResampler& audio_resampler, PlayheadList<AVPacket*>& packet_buffer);
#endif
