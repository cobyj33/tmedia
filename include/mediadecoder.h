#ifndef TMEDIA_MEDIA_DECODER_H
#define TMEDIA_MEDIA_DECODER_H


#include "streamdecoder.h"
#include "boiler.h"
#include "avguard.h"

#include <vector>
#include <string>
#include <set>
#include <filesystem>
#include <mutex>

extern "C" {
  #include <libavutil/avutil.h>
}

/**
 * Acts as a high level interface between a media file and that file's media streams
 * Encapsulates data about the file format and 
 * 
 * Allows seeking
*/
class MediaDecoder {
  private:
    AVFormatContext* fmt_ctx;
    std::unique_ptr<StreamDecoder> decs[AVMEDIA_TYPE_NB];
    MediaType media_type;

    int fetch_next(int requested_packet_count);
  public:
    const std::filesystem::path path;

    MediaDecoder(const std::filesystem::path& file_path, const std::set<enum AVMediaType>& requested_streams);

    MediaDecoder from();


    std::vector<AVFrame*> next_frames(enum AVMediaType media_type); // Not Thread-Safe
    int jump_to_time(double target_time); // Not Thread-Safe
    
    inline double get_duration() const noexcept {
      return this->fmt_ctx->duration / AV_TIME_BASE;
    }
    
    inline MediaType get_media_type() const noexcept {
      return this->media_type;
    }

    inline bool has_stream_decoder(enum AVMediaType media_type) const {
      return this->decs[media_type] != nullptr;
    }

    inline int get_nb_channels() {
      #if HAS_AVCHANNEL_LAYOUT
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout.nb_channels;
      #else
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
      #endif
    }

    inline int get_sample_rate() {
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_rate;
    }

    inline AVSampleFormat get_sample_fmt() {
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_fmt;
    }

    #if HAS_AVCHANNEL_LAYOUT
    inline AVChannelLayout* get_ch_layout() {
      return &(this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout);
    }
    #else
    inline int64_t get_ch_layout() {
      int64_t channel_layout = this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channel_layout;
      if (channel_layout != 0) return channel_layout;

      int channels = this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
      if (channels > 0) return av_get_default_channel_layout(channels);
      return AV_CH_LAYOUT_STEREO;
    }
    #endif



    inline int get_width() {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->width;
    }

    inline int get_height() {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->height;
    }

    inline AVPixelFormat get_pix_fmt() {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->pix_fmt;
    }

    inline double get_start_time(enum AVMediaType media_type) {
      return this->decs[media_type]->get_start_time();
    }

    inline double get_time_base(enum AVMediaType media_type) {
      return this->decs[media_type]->get_time_base();
    }

    inline double get_avgfts(enum AVMediaType media_type) {
      return this->decs[media_type]->get_avgfts();
    }    

    ~MediaDecoder();
};


#endif
