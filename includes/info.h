#ifndef ASCII_VIDEO_INFO
#define ASCII_VIDEO_INFO

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <vector>

extern "C" {
#include <libavutil/avutil.h>
}

typedef struct PacketData {
    enum AVMediaType media_type;
    int packetCount;
} PacketData;

int fileInfoProgram(const char* file_name);

std::vector<PacketData> get_packet_stats(const char* file_name, int* out_data_length);
int get_num_packets(const char* file_name);
#endif
