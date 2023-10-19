#ifndef ASCII_VIDEO_MEDIA_DECODER_H
#define ASCII_VIDEO_MEDIA_DECODER_H


#include "streamdecoder.h"
#include "mediatype.h"
#include "avguard.h"

#include <vector>
#include <string>
#include <set>

extern "C" {
  #include <libavutil/avutil.h>
}

class MediaDecoder {
  private:
    AVFormatContext* format_context;
    std::map<enum AVMediaType, std::unique_ptr<StreamDecoder>> stream_decoders;
    MediaType media_type;

    int fetch_next(int requested_packet_count);
  public:

    MediaDecoder(const std::string& file_path, std::set<enum AVMediaType>& requested_streams);
    bool has_media_decoder(enum AVMediaType media_type) const;
    std::vector<AVFrame*> next_frames(enum AVMediaType media_type);
    int jump_to_time(double target_time);
    double get_duration() const;

    
    MediaType get_media_type();
    double get_start_time(enum AVMediaType media_type);
    int get_avg_frame_time(enum AVMediaType media_type);
    double get_time_base(enum AVMediaType media_type);
    double get_average_frame_time_sec(enum AVMediaType media_type);

    // video getters
    int get_width();
    int get_height();
    AVPixelFormat get_pix_fmt();

    // audio getters
    int get_nb_channels();
    int get_sample_rate();
    AVSampleFormat get_sample_fmt();
    
    #if HAS_AVCHANNEL_LAYOUT
    AVChannelLayout* get_ch_layout();
    #else
    int64_t get_ch_layout();
    #endif
    

    ~MediaDecoder();
};


#endif