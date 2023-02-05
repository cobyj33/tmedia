#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>

#include "info.h"
#include "boiler.h"
#include "streamdata.h"
#include "except.h"

extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}

// int fileInfoProgram(const char* fileName) {
//     av_log_set_level(AV_LOG_INFO);

//     int result;
//     AVFormatContext* formatContext = open_format_context(fileName, &result);

//     av_dump_format(formatContext, 0, fileName, 0);
//     std::vector<PacketData> packetData = get_packet_stats(fileName);
//     for (int i = 0; i < packetData.size(); i++) {
//         std::cout << av_get_media_type_string(packetData[i].mediaType) << " Stream Packet Count: " << packetData[i].packetCount << std::endl;
//     }

//     avformat_close_input(&formatContext);
//     return EXIT_SUCCESS;
// }


// int get_num_packets(const char* fileName) {
//     int result;
//     AVFormatContext* formatContext = open_format_context(fileName, &result);
    
//     int streams_found;
//     enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
//     StreamDataGroup streamData(formatContext, mediaTypes, 2);
//     AVPacket* readingPacket = av_packet_alloc();

//     int count = 0;
//     while (av_read_frame(formatContext, readingPacket) == 0) {
//         count++;
//         av_packet_unref(readingPacket);
//     }

//     avformat_free_context(formatContext);
//     av_packet_free(&readingPacket);
//     return count;
// }

// std::vector<PacketData> get_packet_stats(const char* fileName) {
//     int result;
//     AVFormatContext* formatContext = open_format_context(fileName, &result);
    
//     enum AVMediaType mediaTypes[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
//     StreamDataGroup streamDataGroup(formatContext, mediaTypes, 2);
    
//     AVPacket* readingPacket = av_packet_alloc();
//     std::vector<PacketData> packetData;
//     for (int i = 0; i < streamDataGroup.get_nb_streams(); i++) {
//         packetData.push_back({ streamDataGroup[i].mediaType, 0 });
//     }

//     while (av_read_frame(formatContext, readingPacket) == 0) {
//         for (int i = 0; i < packetData.size(); i++) {
//             if (packetData[i].mediaType == readingPacket->)
//             if (streamDataGroup.has_media_stream(packetData[i].mediaType)) {
//                 packetData[i].packetCount++;
//             }
//         }
//         av_packet_unref(readingPacket);
//     }

//     avformat_free_context(formatContext);
//     av_packet_free(&readingPacket);
//     return packetData;
// }
