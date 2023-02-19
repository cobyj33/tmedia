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

// int fileInfoProgram(const char* file_name) {
//     av_log_set_level(AV_LOG_INFO);

//     int result;
//     AVFormatContext* format_context = open_format_context(file_name, &result);

//     av_dump_format(format_context, 0, file_name, 0);
//     std::vector<PacketData> packetData = get_packet_stats(file_name);
//     for (int i = 0; i < packetData.size(); i++) {
//         std::cout << av_get_media_type_string(packetData[i].media_type) << " Stream Packet Count: " << packetData[i].packetCount << std::endl;
//     }

//     avformat_close_input(&format_context);
//     return EXIT_SUCCESS;
// }


// int get_num_packets(const char* file_name) {
//     int result;
//     AVFormatContext* format_context = open_format_context(file_name, &result);
    
//     int streams_found;
//     enum AVMediaType media_types[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
//     StreamDataGroup streamData(format_context, media_types, 2);
//     AVPacket* reading_packet = av_packet_alloc();

//     int count = 0;
//     while (av_read_frame(format_context, reading_packet) == 0) {
//         count++;
//         av_packet_unref(reading_packet);
//     }

//     avformat_free_context(format_context);
//     av_packet_free(&reading_packet);
//     return count;
// }

// std::vector<PacketData> get_packet_stats(const char* file_name) {
//     int result;
//     AVFormatContext* format_context = open_format_context(file_name, &result);
    
//     enum AVMediaType media_types[2] = { AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_VIDEO };
//     StreamDataGroup streamDataGroup(format_context, media_types, 2);
    
//     AVPacket* reading_packet = av_packet_alloc();
//     std::vector<PacketData> packetData;
//     for (int i = 0; i < streamDataGroup.get_nb_streams(); i++) {
//         packetData.push_back({ streamDataGroup[i].media_type, 0 });
//     }

//     while (av_read_frame(format_context, reading_packet) == 0) {
//         for (int i = 0; i < packetData.size(); i++) {
//             if (packetData[i].media_type == reading_packet->)
//             if (streamDataGroup.has_media_stream(packetData[i].media_type)) {
//                 packetData[i].packetCount++;
//             }
//         }
//         av_packet_unref(reading_packet);
//     }

//     avformat_free_context(format_context);
//     av_packet_free(&reading_packet);
//     return packetData;
// }
