#include "mediadecoder.h"
#include "avguard.h"

#include <stdexcept>
#include <memory>
#include <vector>

#include "boiler.h"
#include "decode.h"
#include "formatting.h"

extern "C" {
  #include <libavutil/avutil.h>
  #include <libavformat/avformat.h>
}

MediaDecoder::MediaDecoder(const char* file_name) {
  try {
    this->format_context = open_format_context(file_name);
  } catch (std::runtime_error const& e) {
    throw std::runtime_error("[MediaDecoder::MediaDecoder] Could not allocate Media Decoder of " + std::string(file_name) + " because of error while fetching file format data: " + e.what());
  } 

  const int NUM_OF_MEDIA_TYPES_TO_SEARCH = 2;
  enum AVMediaType MEDIA_TYPES_TO_SEARCH[NUM_OF_MEDIA_TYPES_TO_SEARCH] = { AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO };

  for (int i = 0; i < NUM_OF_MEDIA_TYPES_TO_SEARCH; i++) {
    try {
      this->stream_decoders[MEDIA_TYPES_TO_SEARCH[i]] = std::move(std::make_unique<StreamDecoder>(format_context, MEDIA_TYPES_TO_SEARCH[i]));
    } catch (std::invalid_argument const& e) {
      continue;
    }
  }
}

MediaDecoder::~MediaDecoder() {
  avformat_close_input(&(this->format_context));
  avformat_free_context(this->format_context);
}

double MediaDecoder::get_duration() const {
  return this->format_context->duration / AV_TIME_BASE;
}

bool MediaDecoder::has_media_decoder(enum AVMediaType media_type) const {
  return this->stream_decoders.count(media_type) == 1;
}

std::vector<AVFrame*> MediaDecoder::next_frames(enum AVMediaType media_type) {
  if (!this->has_media_decoder(media_type)) {
    throw std::runtime_error("[MediaDecoder::next_frames] Cannot get next " +
    std::string(av_get_media_type_string(media_type)) + " frames, as no " + 
    std::string(av_get_media_type_string(media_type)) + " stream is " 
    "available to decode from");
  }

  StreamDecoder& stream_decoder = *(this->stream_decoders[media_type]);
  std::vector<AVFrame*> decoded_frames;
  int fetch_count = -1; 

  do {
    fetch_count = !stream_decoder.has_packets() ? this->fetch_next(10) : -1; // -1 means that no fetch request was made.
    std::vector<AVFrame*> decoded_frames = stream_decoder.decode_next();
    if (decoded_frames.size() > 0) {
      return decoded_frames;
    }
    // no need to clear decoded_frames, if the size of decoded frames is greater than 0, would have already returned
  } while (fetch_count != 0);

  return {}; // no video frames could sadly be found. This should only really ever happen once the file ends
}

int MediaDecoder::fetch_next(int requested_packet_count) {
  AVPacket* reading_packet = av_packet_alloc();
  int packets_read = 0;

  while (av_read_frame(this->format_context, reading_packet) == 0) {
     
    for (auto decoder_pair = this->stream_decoders.begin(); decoder_pair != this->stream_decoders.end(); decoder_pair++) {
      if (decoder_pair->second->get_stream_index() == reading_packet->stream_index) {
        AVPacket* saved_packet = av_packet_alloc();
        av_packet_ref(saved_packet, reading_packet);
        
        decoder_pair->second->push_back_packet(saved_packet);
        packets_read++;
        break;
      }
    }

    av_packet_unref(reading_packet);

     if (packets_read >= requested_packet_count) {
      av_packet_free(&reading_packet);
      return packets_read;
    }
  }

  //reached EOF
  av_packet_free(&reading_packet);
  return packets_read;
}

int MediaDecoder::jump_to_time(double target_time) {
  if (target_time < 0.0 || target_time > this->get_duration()) {
    throw std::runtime_error("Could not jump to time " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(target_time) + " seconds ) "
    ", time is out of the bounds of duration " + format_time_hh_mm_ss(target_time) + " ( " + std::to_string(this->get_duration()) + " seconds )");
  }

  int ret = avformat_seek_file(this->format_context, -1, 0.0, target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

  if (ret < 0) {
    return ret;
  }

  for (auto decoder_pair = this->stream_decoders.begin(); decoder_pair != this->stream_decoders.end(); decoder_pair++) {
    decoder_pair->second->reset();
  }

  for (auto decoder_pair = this->stream_decoders.begin(); decoder_pair != this->stream_decoders.end(); decoder_pair++) {
    std::vector<AVFrame*> frames;
    bool passed_target_time = false;

    do {
      clear_av_frame_list(frames);
      frames = this->next_frames(decoder_pair->first);
      for (int i = 0; i < (int)frames.size(); i++) {
        if (frames[i]->pts * decoder_pair->second->get_time_base() >= target_time) {
          passed_target_time = true;
          break;
        }
      }
    } while (!passed_target_time && frames.size() > 0);
    
    clear_av_frame_list(frames);
  }

  return ret;

}

MediaType MediaDecoder::get_media_type() {
  return this->media_type;
}

/**
 * 
 * Audio Getters 
 * 
*/

int MediaDecoder::get_nb_channels() {
  if (!this->has_media_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error("[MediaDecoder::get_nb_channels] Cannot get number "
    "of audio channels from this media decoder: No audio stream decoder is available");
  }

  #if HAS_AVCHANNEL_LAYOUT
  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->ch_layout.nb_channels;
  #else
  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->channels;
  #endif
}

int MediaDecoder::get_sample_rate() {
  if (!this->has_media_decoder(AVMEDIA_TYPE_AUDIO)) {
    throw std::runtime_error("[MediaDecoder::get_sample_rate] Cannot get sample "
    "rate from this media decoder: No audio stream decoder is available");
  }

  return this->stream_decoders[AVMEDIA_TYPE_AUDIO]->get_codec_context()->sample_rate;
}


/**
 * 
 * Video Getters 
 * 
*/

int MediaDecoder::get_width() {
  if (!this->has_media_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error("[MediaDecoder::get_width] Cannot get width "
    "from this media decoder: No video stream decoder is available");
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->width;
}

int MediaDecoder::get_height() {
  if (!this->has_media_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error("[MediaDecoder::get_height] Cannot get height "
    "from this media decoder: No video stream decoder is available");
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->height;
}

AVPixelFormat MediaDecoder::get_pix_fmt() {
  if (!this->has_media_decoder(AVMEDIA_TYPE_VIDEO)) {
    throw std::runtime_error("[MediaDecoder::get_pix_fmt] Cannot get pixel "
    "format from this media decoder: No video stream decoder is available");
  }

  return this->stream_decoders[AVMEDIA_TYPE_VIDEO]->get_codec_context()->pix_fmt;
}

/**
 * 
 * Miscellaneous Getters 
 * 
*/

double MediaDecoder::get_start_time(enum AVMediaType media_type) {
  if (!this->has_media_decoder(media_type)) {
    throw std::runtime_error("[MediaDecoder::get_start_time] Cannot get start "
    "time from this media decoder: No " + std::string(av_get_media_type_string(media_type)) + " stream decoder is available");
  }

  return this->stream_decoders[media_type]->get_start_time();
}

double MediaDecoder::get_time_base(enum AVMediaType media_type) {
  if (!this->has_media_decoder(media_type)) {
    throw std::runtime_error("[MediaDecoder::get_start_time] Cannot get start "
    "time from this media decoder: No " + std::string(av_get_media_type_string(media_type)) + " stream decoder is available");
  }

  return this->stream_decoders[media_type]->get_time_base();

}

double MediaDecoder::get_average_frame_time_sec(enum AVMediaType media_type) {
  if (!this->has_media_decoder(media_type)) {
    throw std::runtime_error("[MediaDecoder::get_start_time] Cannot get start "
    "time from this media decoder: No " + std::string(av_get_media_type_string(media_type)) + " stream decoder is available");
  }

  return this->stream_decoders[media_type]->get_average_frame_time_sec();
}

StreamDecoder& MediaDecoder::get_stream_decoder(enum AVMediaType media_type) {
  if (!this->has_media_decoder(media_type)) {
    throw std::runtime_error("[MediaDecoder::get_stream_decoder] Cannot get stream "
    "decoder from this media decoder: No " + std::string(av_get_media_type_string(media_type)) + " stream decoder is available");
  }


  return *(this->stream_decoders[media_type]);
}
