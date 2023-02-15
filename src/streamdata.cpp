#include <memory>
#include <stdexcept>

#include "streamdata.h"
#include "except.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

StreamData::StreamData(AVFormatContext* formatContext, enum AVMediaType mediaType) {
    const AVCodec* decoder;
    int streamIndex = -1;
    streamIndex = av_find_best_stream(formatContext, mediaType, -1, -1, &decoder, 0);
    if (streamIndex < 0) {
        throw std::invalid_argument("Cannot find media type for type " + std::string(av_get_media_type_string(mediaType)));
    }

    this->decoder = decoder;
    this->stream = formatContext->streams[streamIndex];
    this->codecContext = avcodec_alloc_context3(this->decoder);

    if (this->codecContext == nullptr) {
        throw std::invalid_argument("Could not create codec context from decoder: " + std::string(decoder->long_name));
    }

    this->mediaType = mediaType;

    int result = avcodec_parameters_to_context(this->codecContext, this->stream->codecpar);
    if (result < 0) {
        throw std::invalid_argument("Could not move AVCodec parameters into context: AVERROR error code " + std::to_string(AVERROR(result)));
    }

    result = avcodec_open2(this->codecContext, this->decoder, NULL);
    if (result < 0) {
        throw std::invalid_argument("Could not initialize AVCodecContext with AVCodec decoder: AVERROR error code " + std::to_string(AVERROR(result)));
    }
};

StreamData::~StreamData() {
    if (this->codecContext != nullptr) {
        avcodec_free_context(&(this->codecContext));
    }
};

/**
 * @brief Construct a new Stream Data Group:: Stream Data Group object
 * BREAKING CHANGE: IF THE MEDIA TYPE IS NOT FOUND, AN ERROR IS THROWN.
 * 
 * @param formatContext 
 * @param mediaTypes 
 * @param nb_target_streams 
 */
StreamDataGroup::StreamDataGroup(AVFormatContext* formatContext, const enum AVMediaType* mediaTypes, int nb_target_streams) {
    for (int i = 0; i < nb_target_streams; i++) {
        std::shared_ptr<StreamData> stream_data_ptr = std::make_shared<StreamData>(formatContext, mediaTypes[i]);
        this->m_datas.push_back(stream_data_ptr);
    }

    this->m_formatContext = formatContext;
};

int StreamDataGroup::get_nb_streams() {
    return this->m_datas.size();
};

StreamData& StreamDataGroup::get_av_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->m_datas.size(); i++) {
        if (this->m_datas[i]->mediaType == media_type) {
            return *this->m_datas[i];
        }
    }

    throw ascii::not_found_error("Could not find stream data for media type " + std::string(av_get_media_type_string(media_type)));
};

bool StreamDataGroup::has_av_media_stream(enum AVMediaType media_type) {
    for (int i = 0; i < this->m_datas.size(); i++) {
        if (this->m_datas[i]->mediaType == media_type) {
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
