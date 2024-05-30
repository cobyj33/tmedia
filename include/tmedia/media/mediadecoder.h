#ifndef TMEDIA_MEDIA_DECODER_H
#define TMEDIA_MEDIA_DECODER_H


#include <tmedia/ffmpeg/streamdecoder.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/avguard.h>
#include <tmedia/util/defines.h>


#include <vector>
#include <array>
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
    std::unique_ptr<AVFormatContext, AVFormatContextDeleter> fmt_ctx;
    std::array<std::unique_ptr<StreamDecoder>, AVMEDIA_TYPE_NB> decs;
    MediaType media_type;

    int fetch_next(int requested_packet_count);
  public:
    const std::filesystem::path path;

    MediaDecoder(const std::filesystem::path& file_path, const std::array<bool, AVMEDIA_TYPE_NB>& requested_streams);

    void next_frames(enum AVMediaType media_type, std::vector<AVFrame*>& frame_buffer); // Not Thread-Safe
    int jump_to_time(double target_time); // Not Thread-Safe
    
    TMEDIA_ALWAYS_INLINE inline double get_duration() const noexcept {
      return this->fmt_ctx->duration / AV_TIME_BASE;
    }
    
    TMEDIA_ALWAYS_INLINE inline MediaType get_media_type() const noexcept {
      return this->media_type;
    }

    TMEDIA_ALWAYS_INLINE inline bool has_stream_decoder(enum AVMediaType media_type) const {
      return this->decs[media_type] != nullptr;
    }

    TMEDIA_ALWAYS_INLINE inline unsigned int nb_stream_decoders() const {
      unsigned int nb = 0;
      for (unsigned int i = 0; i < AVMEDIA_TYPE_NB; i++) {
        nb += this->decs[i] != nullptr;
      }
      return nb;
    }

    TMEDIA_ALWAYS_INLINE inline int get_nb_channels() const {
      #if HAS_AVCHANNEL_LAYOUT
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout.nb_channels;
      #else
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
      #endif
    }

    TMEDIA_ALWAYS_INLINE inline int get_sample_rate() const {
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_rate;
    }

    TMEDIA_ALWAYS_INLINE inline AVSampleFormat get_sample_fmt() const {
      return this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_fmt;
    }

    #if HAS_AVCHANNEL_LAYOUT
    TMEDIA_ALWAYS_INLINE inline AVChannelLayout* get_ch_layout() const {
      return &(this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout);
    }
    #else
    TMEDIA_ALWAYS_INLINE inline int64_t get_ch_layout() const {
      int64_t channel_layout = this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channel_layout;
      if (channel_layout != 0) return channel_layout;

      int channels = this->decs[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
      if (channels > 0) return av_get_default_channel_layout(channels);
      return AV_CH_LAYOUT_STEREO;
    }
    #endif

    TMEDIA_ALWAYS_INLINE inline int get_width() const {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->width;
    }

    TMEDIA_ALWAYS_INLINE inline int get_height() const {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->height;
    }

    TMEDIA_ALWAYS_INLINE inline AVPixelFormat get_pix_fmt() const {
      return this->decs[AVMEDIA_TYPE_VIDEO]->get_codec_context()->pix_fmt;
    }

    TMEDIA_ALWAYS_INLINE inline double get_start_time(enum AVMediaType media_type) const {
      return this->decs[media_type]->get_start_time();
    }

    TMEDIA_ALWAYS_INLINE inline double get_time_base(enum AVMediaType media_type) const {
      return this->decs[media_type]->get_time_base();
    }

    TMEDIA_ALWAYS_INLINE inline double get_avgfts(enum AVMediaType media_type) const {
      return this->decs[media_type]->get_avgfts();
    }    
};


#endif
