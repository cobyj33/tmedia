#ifndef ASCII_VIDEO_DEBUG
#define ASCII_VIDEO_DEBUG
#include "boiler.h"


#define MEDIA_DEBUG_MESSAGE_BUFFER_SIZE 1000


typedef struct DebugMessage {
    char* message;
    int size;
    int capacity;
    const char* source;
    const char* type;
    const char* desc;
} DebugMessage;

typedef struct media_debug_info {
    DebugMessage* messages[MEDIA_DEBUG_MESSAGE_BUFFER_SIZE];
    int nb_messages;
} MediaDebugInfo;

void add_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, ...);
void append_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, ...);
void clear_media_debug(MediaDebugInfo* debug, const char* source, const char* type);
void remove_debug_message(MediaDebugInfo* debug, const char* source, const char* type, const char* desc);
MediaDebugInfo* media_debug_info_alloc();
void media_debug_info_free(MediaDebugInfo* info);
#endif
