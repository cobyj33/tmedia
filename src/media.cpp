#include "doublelinkedlist.hpp"
#include "icons.h"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <media.h>
#include <stdarg.h>

void video_symbol_stack_push(VideoSymbolStack* stack, VideoSymbol *symbol) {
    if (stack->top < VIDEO_SYMBOL_BUFFER_SIZE - 1) {
        stack->top++;
        stack->symbols[stack->top] = symbol;
    } else {
        free_video_symbol(stack->symbols[VIDEO_SYMBOL_BUFFER_SIZE - 1]);
        stack->symbols[VIDEO_SYMBOL_BUFFER_SIZE - 1] = symbol;
    }
}

void video_symbol_stack_erase_pop(VideoSymbolStack* stack) {
    if (stack->top >= 0) {
        free_video_symbol(stack->symbols[stack->top]);
        stack->top--;
    }
}

VideoSymbol* video_symbol_stack_pop(VideoSymbolStack* stack) {
    if (stack->top >= 0) {
        VideoSymbol* copiedSymbol = copy_video_symbol(stack->symbols[stack->top]);
        free_video_symbol(stack->symbols[stack->top]);
        stack->top--;
        return copiedSymbol;
    }

    return nullptr;
}

VideoSymbol* video_symbol_stack_peek(VideoSymbolStack* stack) {
    if (stack->top >= 0) {
        return stack->symbols[stack->top];    
    }
    return nullptr;
}

void video_symbol_stack_clear(VideoSymbolStack* stack) {
    for (int i = stack->top; i >= 0; i++) {
        free_video_symbol(stack->symbols[i]);
    }

    stack->top = -1;
}

MediaStream* get_media_stream(MediaData* media_data, AVMediaType media_type) {
    for (int i = 0; i < media_data->nb_streams; i++) {
        if (media_data->media_streams[i]->info->mediaType == media_type) {
            return media_data->media_streams[i];
        }
    }
    return nullptr;
}

bool has_media_stream(MediaData* media_data, AVMediaType media_type) {
    return get_media_stream(media_data, media_type) != nullptr;
}

MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode) {
    for (int i = 0; i < nb_display_modes; i++) {
        if (display_modes[i] == currentMode) {
            return display_modes[i + 1 % nb_display_modes];
        }
    }
    return currentMode;
}

void add_debug_message(MediaDebugInfo* debug_info, const char* format, ...) {
    if (debug_info->nb_messages >= MEDIA_DEBUG_MESSAGE_BUFFER_SIZE) {
        return;
    }

    va_list args;
    va_start(args, format);
    std::size_t alloc_size = std::vsnprintf(NULL, 0, format, args);
    char* string = (char*)malloc(alloc_size);
    std::vsnprintf(string, alloc_size, format, args);
    debug_info->messages[debug_info->nb_messages] = string;
    debug_info->nb_messages++;
    va_end(args);
}

void clear_media_debug(MediaDebugInfo* debug) {
    for (int i = 0; i < debug->nb_messages; i++) {
        free(debug->messages[i]);
    }
    debug->nb_messages = 0; 
}

void move_packet_list_to_pts(DoubleLinkedList<AVPacket*>* packets, int64_t targetPTS) {
    bool result = true;
    int64_t lastPTS = packets->get()->pts;

    while (true) {
        int64_t currentIndexPTS = packets->get()->pts;
        if (packets->get_index() == 0 || packets->get_index() == packets->get_length() - 1) {
            break;
        } 

        if (currentIndexPTS > targetPTS) {
            result = packets->set_index(packets->get_index() - 1);
            if (packets->get()->pts < targetPTS) {
                break;
            }
        } else if (currentIndexPTS < targetPTS) {
            result = packets->set_index(packets->get_index() + 1);
            if (packets->get()->pts > targetPTS) {
                packets->set_index(packets->get_index() - 1);
                break;
            }
        } else if (currentIndexPTS == targetPTS) {
            break;
        }

        if (!result) {
            break;
        }

        currentIndexPTS = packets->get()->pts;

        if (std::min(currentIndexPTS, lastPTS) <= targetPTS && std::max(currentIndexPTS, lastPTS) >= targetPTS) {
            break;
        }

        lastPTS = currentIndexPTS;
    } 
}