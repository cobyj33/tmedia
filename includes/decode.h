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


// void free_frame_list(std::vector<AVFrame*> frameList, int count);
/**
 * @brief clears the AVFrame vector of all it's contents **AND FREES ALL AVFRAME's CONTAINED WITHIN**
 * @param frameList 
 */
void clear_av_frame_list(std::vector<AVFrame*>& frameList);

std::vector<AVFrame*> decode_video_packet(AVCodecContext* videoCodecContext, AVPacket* videoPacket);
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audioCodecContext, AVPacket* audioPacket);
std::vector<AVFrame*> decode_video_packet(AVCodecContext* videoCodecContext, PlayheadList<AVPacket*>& videoPacketList);
std::vector<AVFrame*> decode_audio_packet(AVCodecContext* audioCodecContext, PlayheadList<AVPacket*>& audioPacketList);

std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler& audioResampler, AVPacket* packet);
std::vector<AVFrame*> get_final_audio_frames(AVCodecContext* audioCodecContext, AudioResampler& audioResampler, PlayheadList<AVPacket*>& packet_buffer);
#endif
