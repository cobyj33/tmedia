#include "debug.h"
#include <string.h>
#include <media.h>

void vadd_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, va_list args);
char* get_formatted_string(int* output_size, const char* format, ...);
char* vget_formatted_string(int* output_size, const char* format, va_list args);

int find_debug_message(MediaDebugInfo* debug_info, const char* source, const char* type, const char* desc);
int has_debug_message(MediaDebugInfo* debug_info, const char* source, const char* type, const char* desc);

const MediaDisplayMode display_modes[NUMBER_OF_DISPLAY_MODES] = { DISPLAY_MODE_VIDEO, DISPLAY_MODE_AUDIO };

MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode) {
    for (int i = 0; i < NUMBER_OF_DISPLAY_MODES; i++) {
        if (display_modes[i] == currentMode) {
            return display_modes[(i + 1) % NUMBER_OF_DISPLAY_MODES];
        }
    }
    return currentMode;
}


void append_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, ...) {
    int to_replace = find_debug_message(debug_info, message_source, message_type, message_desc);
    va_list args;
    va_start(args, format);
    if (to_replace == -1) {
        vadd_debug_message(debug_info, message_source, message_type, message_desc, format, args);
    } else {
        DebugMessage* target = debug_info->messages[to_replace];
        int string_size;
        char* string = vget_formatted_string(&string_size, format, args);
        if (string == NULL) {
            va_end(args);
            return;
        }

        if (target->size + string_size + 1 > target->capacity) {
            char* tmp = (char*)realloc(target->message, target->capacity * 2);
            if (tmp != NULL) {
                target->message = tmp;
                target->capacity *= 2;
            }
        }

        if (target->size + string_size + 1 < target->capacity) {
            for (int i = 0; i < string_size; i++) {
                target->message[target->size + i] = string[i]; 
            }
            target->message[target->size + string_size] = '\0';
            target->size += string_size;
        }

        free(string);
    }

    va_end(args);
}

void add_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vadd_debug_message(debug_info, message_source, message_type, message_desc, format, args);
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

void remove_debug_message(MediaDebugInfo* debug, const char* source, const char* type, const char* desc) {
    int to_replace = find_debug_message(debug, source, type, desc);
    if (to_replace != -1) {
        free(debug->messages[to_replace]->message);
        free(debug->messages[to_replace]);
        for (int i = to_replace + 1; i < debug->nb_messages; i++) {
            debug->messages[i - 1] = debug->messages[i];
        }
        debug->nb_messages--;
    }

}

int find_debug_message(MediaDebugInfo* debug_info, const char* source, const char* type, const char* desc) {
    for (int i = 0; i < debug_info->nb_messages; i++) {
        const DebugMessage* current_message = debug_info->messages[i];
        if (strcmp(source, current_message->source) == 0 && strcmp(type, current_message->type) == 0 && strcmp(desc, current_message->desc) == 0) {
            return i;
        }
    }
    return -1;
}

int has_debug_message(MediaDebugInfo* debug_info, const char* source, const char* type, const char* desc) {
    return find_debug_message(debug_info, source, type, desc) == -1 ? 0 : 1;
}

char* get_formatted_string(int* output_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    char* string = vget_formatted_string(output_size, format, args);
    va_end(args);
    return string;
}


char* vget_formatted_string(int* output_size, const char* format, va_list args) {
    va_list writing_args, reading_args;
    va_copy(reading_args, args);
    va_copy(writing_args, reading_args);

    size_t alloc_size = vsnprintf(NULL, 0, format, reading_args);
    va_end(reading_args);
    char* string = (char*)malloc(sizeof(char) * (alloc_size + 1));

    if (string == NULL) {
        va_end(writing_args);
        return NULL;
    }

    *output_size = vsnprintf(string, alloc_size + 1, format, writing_args);
    va_end(writing_args);
    return string;
}

void vadd_debug_message(MediaDebugInfo* debug_info, const char* message_source, const char* message_type, const char* message_desc, const char* format, va_list args) {
    int to_replace = find_debug_message(debug_info, message_source, message_type, message_desc);
    if (debug_info->nb_messages >= MEDIA_DEBUG_MESSAGE_BUFFER_SIZE && to_replace == -1) {
        return;
    }

    va_list writing_args, reading_args;
    va_copy(reading_args, args);
    va_copy(writing_args, args);

    size_t alloc_size = vsnprintf(NULL, 0, format, reading_args);
    va_end(reading_args);
    int capacity = (sizeof(char) * (alloc_size) + 1) * 2;
    char* string = (char*)malloc(capacity);
    if (string == NULL) {
        va_end(writing_args);
        return;
    }

    vsnprintf(string, alloc_size + 1, format, writing_args);
    va_end(writing_args);

    if (to_replace == -1) {
        DebugMessage* debug_message = (DebugMessage*)malloc(sizeof(DebugMessage));
        debug_message->source = message_source;
        debug_message->type = message_type;
        debug_message->desc = message_desc;

        debug_message->message = string;
        debug_message->size = alloc_size;
        debug_message->capacity = capacity;
        debug_info->messages[debug_info->nb_messages] = debug_message;
        debug_info->nb_messages++;
    } else {
        free(debug_info->messages[to_replace]->message);
        debug_info->messages[to_replace]->message = string;
        debug_info->messages[to_replace]->size = alloc_size;
        debug_info->messages[to_replace]->capacity = capacity;
    }
}
