#include <cstdlib>
#include <info.h>
#include <iostream>
#include <boiler.h>
#include <libavutil/log.h>
extern "C" {
    #include <libavformat/avformat.h>
}

int fileInfoProgram(const char* fileName) {
    av_log_set_level(AV_LOG_INFO);

    int result;
    AVFormatContext* formatContext = open_format_context(fileName, &result);
    if (formatContext == nullptr) {
        std::cerr << "COULD NOT LOAD FILE INFORMATION ABOUT " << fileName << std::endl;
        return EXIT_FAILURE;
    } else if (result < 0) {
        char errBuf[512];
        av_strerror(result, errBuf, 512);
        std::cerr << "ERROR WHILE OPENING FORMAT CONTEXT: \n    " << errBuf << "\n" << std::endl;
    }

    av_dump_format(formatContext, 0, fileName, 0);

    int streams_found;
    PacketData* packetData = get_packet_stats(fileName, &streams_found);
    std::cout << std::endl;
    for (int i = 0; i < streams_found; i++) {
        std::cout << av_get_media_type_string(packetData[i].mediaType) << " Stream Packet Count: " << packetData[i].packetCount << std::endl;
    }
    free(packetData);

    avformat_close_input(&formatContext);
    return EXIT_SUCCESS;
}


int get_num_packets(const char* fileName) {
    int result;
    AVFormatContext* formatContext = open_format_context(fileName, &result);
    if (formatContext == nullptr) {
        return 0;
    } else if (result < 0) {
        avformat_free_context(formatContext);
        return 0;
    }
    
    int streams_found;
    /* AVMediaType mediaTypes[3] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_SUBTITLE }; */
    /* StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 3, &streams_found); */
    AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 2, &streams_found);
    if (streamData == nullptr || streams_found == 0) {
        std::cerr << "Could not fetch packet stats: error while fetching stream data of " << fileName << std::endl;
        avformat_free_context(formatContext);
        return 0;
    }
    
    AVPacket* readingPacket = av_packet_alloc();

    int count = 0;
    while (av_read_frame(formatContext, readingPacket) == 0) {
        count++;
        av_packet_unref(readingPacket);
    }

    avformat_free_context(formatContext);
    av_packet_free(&readingPacket);
    stream_datas_free(streamData, streams_found);
    return count;
}

PacketData* get_packet_stats(const char* fileName, int* streams_found) {
    int result;
    *streams_found = 0;
    AVFormatContext* formatContext = open_format_context(fileName, &result);
    if (formatContext == nullptr) {
        return nullptr;
    } else if (result < 0) {
        avformat_free_context(formatContext);
        return nullptr;
    }
    
    /* AVMediaType mediaTypes[3] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_SUBTITLE }; */
    /* StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 3, streams_found); */

    AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 2, streams_found);

    if (streamData == nullptr) {
        std::cerr << "Could not fetch packet stats: error while fetching stream data of " << fileName << std::endl;
        avformat_free_context(formatContext);
        return nullptr;
    }
    
    AVPacket* readingPacket = av_packet_alloc();
    PacketData* packetData = (PacketData*)malloc(sizeof(PacketData) * *streams_found);
    for (int i = 0; i < *streams_found; i++) {
        packetData[i] = { streamData[i]->mediaType, 0 };
    }

    while (av_read_frame(formatContext, readingPacket) == 0) {
        for (int i = 0; i < *streams_found; i++) {
            if (packetData[i].mediaType == streamData[i]->mediaType) {
                packetData[i].packetCount++;
            }
        }
        av_packet_unref(readingPacket);
    }

    avformat_free_context(formatContext);
    av_packet_free(&readingPacket);
    stream_datas_free(streamData, *streams_found);
    return packetData;
}
