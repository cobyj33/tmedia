#include <memory>
#include <stdexcept>

#include "streamdata.h"
#include "except.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

StreamData::StreamData(AVFormatContext* format_context, enum AVMediaType media_type) {
    const AVCodec* decoder;
    int stream_index = -1;
    stream_index = av_find_best_stream(format_context, media_type, -1, -1, &decoder, 0);
    if (stream_index < 0) {
        throw std::invalid_argument("Cannot find media type for type " + std::string(av_get_media_type_string(media_type)));
    }

    this->decoder = decoder;
    this->stream = format_context->streams[stream_index];
    this->codec_context = avcodec_alloc_context3(this->decoder);

    if (this->codec_context == nullptr) {
        throw std::invalid_argument("Could not create codec context from decoder: " + std::string(decoder->long_name));
    }

    this->media_type = media_type;

    int result = avcodec_parameters_to_context(this->codec_context, this->stream->codecpar);
    if (result < 0) {
        throw std::invalid_argument("Could not move AVCodec parameters into context: AVERROR error code " + std::to_string(AVERROR(result)));
    }

    result = avcodec_open2(this->codec_context, this->decoder, NULL);
    if (result < 0) {
        throw std::invalid_argument("Could not initialize AVCodecContext with AVCodec decoder: AVERROR error code " + std::to_string(AVERROR(result)));
    }
};

StreamData::~StreamData() {
    if (this->codec_context != nullptr) {
        avcodec_free_context(&(this->codec_context));
    }
};

/**
 * @brief Construct a new Stream Data Group:: Stream Data Group object
 * BREAKING CHANGE: IF THE MEDIA TYPE IS NOT FOUND, AN ERROR IS THROWN.
 * 
 * @param format_context 
 * @param media_types 
 * @param nb_target_streams 
 */
StreamDataGroup::StreamDataGroup(AVFormatContext* format_context, const enum AVMediaType* media_types, int nb_target_streams) {
    for (int i = 0; i < nb_target_streams; i++) {
        std::shared_ptr<StreamData> stream_data_ptr = std::make_shared<StreamData>(format_context, media_types[i]);
        this->m_datas.push_back(stream_data_ptr);
    }

    this->m_format_context = format_context;
};

int StreamDataGroup::get_nb_streams() {
    return this->m_datas.size();
};

StreamData& StreamDataGroup::get_av_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->m_datas.size(); i++) {
        if (this->m_datas[i]->media_type == media_type) {
            return *this->m_datas[i];
        }
    }

    throw ascii::not_found_error("Could not find stream data for media type " + std::string(av_get_media_type_string(media_type)));
};

bool StreamDataGroup::has_av_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->m_datas.size(); i++) {
        if (this->m_datas[i]->media_type == media_type) {
            return true;
        }
    }
    return false;
};

StreamData& StreamDataGroup::operator[](int index)
{
    if (index < 0 || index >= this->m_datas.size()) {
        throw std::out_of_range("Attempted to access StreamData in StreamDataGroup from out of bounds index: " + std::to_string(index) + " ( length: " + std::to_string(this->m_datas.size())  + " )");
    }

    return *this->m_datas[index];
}
