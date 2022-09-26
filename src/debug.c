#include "debug.h"
#include <string.h>
#include <media.h>

const MediaDisplayMode display_modes[NUMBER_OF_DISPLAY_MODES] = { DISPLAY_MODE_VIDEO, DISPLAY_MODE_AUDIO };

MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode) {
    for (int i = 0; i < NUMBER_OF_DISPLAY_MODES; i++) {
        if (display_modes[i] == currentMode) {
            return display_modes[i + 1 % NUMBER_OF_DISPLAY_MODES];
        }
    }
    return currentMode;
}


void add_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* format, ...) {
    if (debug_info->nb_messages >= MEDIA_DEBUG_MESSAGE_BUFFER_SIZE) {
        return;
    }

    va_list args;
    va_start(args, format);
    size_t alloc_size = vsnprintf(NULL, 0, format, args);
    char* string = (char*)malloc(alloc_size);
    if (string != NULL) {
        DebugMessage* debug_message = (DebugMessage*)malloc(sizeof(DebugMessage));
        debug_message->size = alloc_size;
        debug_message->source = message_source;
        debug_message->type = message_type;

        vsnprintf(string, alloc_size, format, args);
        debug_message->message = string;
        debug_info->messages[debug_info->nb_messages] = debug_message;
        debug_info->nb_messages++;
    }

    va_end(args);
}

void clear_media_debug(MediaDebugInfo* debug, const char* source, const char* type) {
    DebugMessage* saved_messages[debug->nb_messages];
    int nb_saved_messages = 0;
    for (int i = 0; i < debug->nb_messages; i++) {
        if (( strlen(source) == 0 || strcmp(debug->messages[i]->source, source) == 0) && (strlen(type) == 0 || strcmp(debug->messages[i]->type, type) == 0)) {
            free(debug->messages[i]->message);
            free(debug->messages[i]);
        } else {
            saved_messages[nb_saved_messages] = debug->messages[i];
            nb_saved_messages++;
        }
    }

    debug->nb_messages = nb_saved_messages;
    for (int i = 0; i < nb_saved_messages; i++) {
        debug->messages[i] = saved_messages[i];
    }
}
