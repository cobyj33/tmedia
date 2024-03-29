#ifndef TMEDIA_MEDIA_DECODER_H
#define TMEDIA_MEDIA_DECODER_H


#include "streamdecoder.h"
#include "boiler.h"
#include "avguard.h"

#include <vector>
#include <string>
#include <set>
#include <filesystem>

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
    std::map<enum AVMediaType, std::unique_ptr<StreamDecoder>> stream_decoders;
    MediaType media_type;
    std::map<std::string, std::string> metadata;

    int fetch_next(int requested_packet_count);
  public:

    MediaDecoder(std::filesystem::path file_path, std::set<enum AVMediaType>& requested_streams);
    bool has_stream_decoder(enum AVMediaType media_type) const; // Thread-Safe

    std::vector<AVFrame*> next_frames(enum AVMediaType media_type); // Not Thread-Safe
    int jump_to_time(double target_time); // Not Thread-Safe
    
    double get_duration() const; // Thread-Safe

    
    MediaType get_media_type(); // Thread-Safe
    double get_start_time(enum AVMediaType media_type); // Thread-Safe
    int get_avg_frame_time(enum AVMediaType media_type); // Thread-Safe
    double get_time_base(enum AVMediaType media_type); // Thread-Safe
    double get_average_frame_time_sec(enum AVMediaType media_type); // Thread-Safe

    // video getters
    int get_width(); // Thread-Safe
    int get_height(); // Thread-Safe
    AVPixelFormat get_pix_fmt(); // Thread-Safe

    // audio getters
    int get_nb_channels(); // Thread-Safe
    int get_sample_rate(); // Thread-Safe
    AVSampleFormat get_sample_fmt(); // Thread-Safe
    
    #if HAS_AVCHANNEL_LAYOUT
    AVChannelLayout* get_ch_layout(); // Thread-Safe
    #else
    int64_t get_ch_layout(); // Thread-Safe
    #endif
    

    ~MediaDecoder();
};


#endif
