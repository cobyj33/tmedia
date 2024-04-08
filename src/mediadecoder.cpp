#include "mediadecoder.h"

#include "avguard.h"
#include "boiler.h"
#include "decode.h"
#include "formatting.h"
#include "funcmac.h"
#include "ffmpeg_error.h"

#include <string>
#include <vector>
#include <set>
#include <stdexcept> 
#include <memory> //std::make_unique
#include <utility>
#include <filesystem>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/avutil.h>
  #include <libavformat/avformat.h>
}

MediaDecoder::MediaDecoder(const std::filesystem::path& path, std::set<enum AVMediaType>& requested_streams) {
  try {
    this->fmt_ctx = open_format_context(path);
  } catch (std::runtime_error const& e) {
    throw std::runtime_error(fmt::format("[{}] Could not allocate Media "
    "Decoder of {} because of error while fetching file format data: {}",
    FUNCDINFO, path.c_str(), + e.what()));
  }

  this->media_type = media_type_from_avformat_context(this->fmt_ctx);

  for (const enum AVMediaType& stream_type : requested_streams) {
    try {
      this->stream_decoders[stream_type] = std::move(std::make_unique<StreamDecoder>(fmt_ctx, stream_type));
    } catch (std::runtime_error const& e) {
      continue;
    }
  }
}

MediaDecoder::~MediaDecoder() {
  avformat_close_input(&(this->fmt_ctx));
}



std::vector<AVFrame*> MediaDecoder::next_frames(enum AVMediaType media_type) {
  if (!this->has_stream_decoder(media_type)) {
    throw std::runtime_error(fmt::format("[{0}] Cannot get next {1} frames, "
    "as no {1} stream is available to decode from",
    FUNCDINFO, av_get_media_type_string(media_type)));
  }

  constexpr int NO_FETCH_MADE = -1;

  StreamDecoder& stream_decoder = *(this->stream_decoders[media_type]);
  std::vector<AVFrame*> dec_frames;
  int fetch_count = NO_FETCH_MADE; 

  do {
    fetch_count = !stream_decoder.has_packets() ? this->fetch_next(10) : NO_FETCH_MADE;
    dec_frames = stream_decoder.decode_next();
    if (dec_frames.size() > 0) {
      return dec_frames;
    }
    // no need to clear dec_frames, if the size of decoded frames is greater than 0, would have already returned
  } while (fetch_count > 0 || fetch_count == NO_FETCH_MADE);

  return dec_frames; // no video frames could sadly be found. This should only really ever happen once the file ends
}

int MediaDecoder::fetch_next(int requested_packet_count) {
  int packets_read = 0;
  AVPacket* reading_packet = av_packet_alloc();
  if (reading_packet == nullptr) {
    throw ffmpeg_error(fmt::format("[{}] Failed to allocate AVPacket", FUNCDINFO), AVERROR(ENOMEM));
  }

  while (av_read_frame(this->fmt_ctx, reading_packet) == 0) {
     
    for (auto &decoder_pair : this->stream_decoders) {
      if (decoder_pair.second->get_stream_index() == reading_packet->stream_index) {
        AVPacket* saved_packet = av_packet_alloc();
        if (saved_packet == nullptr) {
          av_packet_free(&reading_packet);
          throw ffmpeg_error(fmt::format("[{}] Failed to allocate AVPacket", FUNCDINFO), AVERROR(ENOMEM));
        }

        av_packet_ref(saved_packet, reading_packet);
        
        decoder_pair.second->push_back(saved_packet);
        packets_read++;
        break;
      }
    }

    av_packet_unref(reading_packet);

     if (packets_read >= requested_packet_count) break;
  }

  //reached EOF
  av_packet_free(&reading_packet);
  return packets_read;
}

int MediaDecoder::jump_to_time(double target_time) {
  if (target_time < 0.0 || target_time > this->get_duration()) {
    throw std::runtime_error(fmt::format("[{}]Could not jump to time {} ({} "
    "seconds), time is out of the bounds of duration {} ({} seconds)",
    FUNCDINFO, format_time_hh_mm_ss(target_time), target_time,
    format_time_hh_mm_ss(this->get_duration()), this->get_duration()));
  }

  int ret = avformat_seek_file(this->fmt_ctx, -1, 0.0,
    target_time * AV_TIME_BASE, target_time * AV_TIME_BASE, 0);

  if (ret < 0) {
    return ret;
  }

  for (auto& decoder_pair : this->stream_decoders) {
    decoder_pair.second->reset();
  }

  for (auto& decoder_pair : this->stream_decoders) {
    std::vector<AVFrame*> frames;
    bool passed_target_time = false;

    do {
      clear_avframe_list(frames);
      frames = this->next_frames(decoder_pair.first);
      for (std::size_t i = 0; i < frames.size(); i++) {
        if (frames[i]->pts * decoder_pair.second->get_time_base() >= target_time) {
          passed_target_time = true;
          break;
        }
      }
    } while (!passed_target_time && frames.size() > 0);
    
    clear_avframe_list(frames);
  }

  return ret;
}
