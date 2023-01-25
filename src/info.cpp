#include <cstdlib>
#include <cstdio>
#include <info.h>
#include <boiler.h>
#include <streamdata.h>

extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}

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
    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamDataGroup streamData(formatContext, mediaTypes, 2);
    AVPacket* readingPacket = av_packet_alloc();

    int count = 0;
    while (av_read_frame(formatContext, readingPacket) == 0) {
        count++;
        av_packet_unref(readingPacket);
    }

    avformat_free_context(formatContext);
    av_packet_free(&readingPacket);
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
    
    enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
    StreamDataGroup streamDataGroup(formatContext, mediaTypes, 2);
    *streams_found = streamDataGroup.get_nb_streams();
    
    AVPacket* readingPacket = av_packet_alloc();
    PacketData* packetData = (PacketData*)malloc(sizeof(PacketData) * *streams_found);
    for (int i = 0; i < *streams_found; i++) {
        packetData[i] = (PacketData){ streamDataGroup[i]->mediaType, 0 };
    }

    while (av_read_frame(formatContext, readingPacket) == 0) {
        for (int i = 0; i < streamDataGroup.get_nb_streams(); i++) {
            if (streamDataGroup.has_media_stream(packetData[i].mediaType)) {
                packetData[i].packetCount++;
            }
        }
        av_packet_unref(readingPacket);
    }

    avformat_free_context(formatContext);
    av_packet_free(&readingPacket);
    return packetData;
}
