#pragma once

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

extern "C" {
#include <libavutil/avutil.h>
}

typedef struct PacketData {
    AVMediaType mediaType;
    int packetCount;
} PacketData;

int fileInfoProgram(const char* fileName);

PacketData* get_packet_stats(const char* fileName, int* out_data_length);
int get_num_packets(const char* fileName);
