#ifndef ASCII_VIDEO_DEBUG
#define ASCII_VIDEO_DEBUG
#include "boiler.h"

#define MEDIA_DEBUG_MESSAGE_BUFFER_SIZE 100


typedef struct DebugMessage {
    char* message;
    int size;
    const char* source;
    const char* type;
} DebugMessage;

typedef struct media_debug_info {
    DebugMessage* messages[MEDIA_DEBUG_MESSAGE_BUFFER_SIZE];
    int nb_messages;
} MediaDebugInfo;

void add_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* format, ...);
void clear_media_debug(MediaDebugInfo* debug, const char* source, const char* type);
MediaDebugInfo* media_debug_info_alloc();
void media_debug_info_free(MediaDebugInfo* info);
#endif
