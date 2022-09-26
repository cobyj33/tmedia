#include <stdlib.h>
#include <stdio.h>
#include <info.h>
#include <boiler.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

int fileInfoProgram(const char* fileName) {
    av_log_set_level(AV_LOG_INFO);

    int result;
    AVFormatContext* formatContext = open_format_context(fileName, &result);
    if (formatContext == NULL) {
        fprintf(stderr, "%s %s\n", "COULD NOT LOAD FILE INFORMATION ABOUT", fileName);
        return EXIT_FAILURE;
    } else if (result < 0) {
        char errBuf[512];
        av_strerror(result, errBuf, 512);
        fprintf(stderr, "%s\n   %s\n" ,"ERROR WHILE OPENING FORMAT CONTEXT:" , errBuf);
    }

    av_dump_format(formatContext, 0, fileName, 0);
    int streams_found;
    PacketData* packetData = get_packet_stats(fileName, &streams_found);
    for (int i = 0; i < streams_found; i++) {
        printf("%s %s %d", av_get_media_type_string(packetData[i].mediaType), " Stream Packet Count: ", packetData[i].packetCount);
    }
    free(packetData);

    avformat_close_input(&formatContext);
    return EXIT_SUCCESS;
}


int get_num_packets(const char* fileName) {
    int result;
    AVFormatContext* formatContext = open_format_context(fileName, &result);
    if (formatContext == NULL) {
        return 0;
    } else if (result < 0) {
        avformat_free_context(formatContext);
        return 0;
    }
    
    int streams_found;
    /* AVMediaType mediaTypes[3] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_SUBTITLE }; */
    /* StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 3, &streams_found); */
    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 2, &streams_found);
    if (streamData == NULL || streams_found == 0) {
        fprintf(stderr, "%s %s\n" ,"Could not fetch packet stats: error while fetching stream data of ", fileName);
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
    if (formatContext == NULL) {
        return NULL;
    } else if (result < 0) {
        avformat_free_context(formatContext);
        return NULL;
    }
    
    /* AVMediaType mediaTypes[3] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_SUBTITLE }; */
    /* StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 3, streams_found); */

    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamData** streamData = alloc_stream_datas(formatContext, mediaTypes, 2, streams_found);

    if (streamData == NULL) {
        fprintf(stderr, "%s %s\n", "Could not fetch packet stats: error while fetching stream data of ", fileName);
        avformat_free_context(formatContext);
        return NULL;
    }
    
    AVPacket* readingPacket = av_packet_alloc();
    PacketData* packetData = (PacketData*)malloc(sizeof(PacketData) * *streams_found);
    for (int i = 0; i < *streams_found; i++) {
        packetData[i] = (PacketData){ streamData[i]->mediaType, 0 };
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
