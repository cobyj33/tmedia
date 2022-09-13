#pragma once
#include "boiler.h"


#define MEDIA_DEBUG_MESSAGE_BUFFER_SIZE 100

typedef struct DebugMessage {
    char* message;
    int size;
    const char* source;
    const char* type;
} DebugMessage;

typedef struct MediaDebugInfo {
    char* messages[MEDIA_DEBUG_MESSAGE_BUFFER_SIZE];
    int nb_messages;
} MediaDebugInfo;

void add_debug_message(MediaDebugInfo* debug_info, const char* format, ...);
void clear_media_debug(MediaDebugInfo* debug);

