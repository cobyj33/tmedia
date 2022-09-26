#include "icons.h"
#include "selectionlist.h"
#include "wtime.h"
#include <stddef.h>
#include <stdint.h>
#include <media.h>
#include <stdarg.h>
#include <wmath.h>

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

    return NULL;
}

VideoSymbol* video_symbol_stack_peek(VideoSymbolStack* stack) {
    if (stack->top >= 0) {
        return stack->symbols[stack->top];    
    }
    return NULL;
}

void video_symbol_stack_clear(VideoSymbolStack* stack) {
    for (int i = stack->top; i >= 0; i++) {
        free_video_symbol(stack->symbols[i]);
    }

    stack->top = -1;
}

MediaStream* get_media_stream(MediaData* media_data, enum AVMediaType media_type) {
    for (int i = 0; i < media_data->nb_streams; i++) {
        if (media_data->media_streams[i]->info->mediaType == media_type) {
            return media_data->media_streams[i];
        }
    }
    return NULL;
}

int has_media_stream(MediaData* media_data, enum AVMediaType media_type) {
    return get_media_stream(media_data, media_type) == NULL ? 0 : 1;
}

double get_playback_current_time(Playback* playback) {
    return clock_sec() - playback->start_time - playback->paused_time + playback->skipped_time; 
}


void move_packet_list_to_pts(SelectionList* packets, int64_t targetPTS) {
    int result = 1;
    if (selection_list_length(packets) == 0) {
        return;
    }

    AVPacket* current = (AVPacket*)selection_list_get(packets);
    int64_t lastPTS = current->pts;

    while (1) {
        current = (AVPacket*)selection_list_get(packets);

        int64_t currentIndexPTS = current->pts;
        if (selection_list_index(packets) == 0 || selection_list_index(packets) == selection_list_length(packets) - 1) {
            break;
        } 

        if (currentIndexPTS > targetPTS) {
            result = selection_list_try_move_index(packets, -1);
            current = (AVPacket*)selection_list_get(packets);
            if (current->pts < targetPTS) {
                break;
            }
        } else if (currentIndexPTS < targetPTS) {
            result = selection_list_try_move_index(packets, 1);
            current = (AVPacket*)selection_list_get(packets);
            if (current->pts > targetPTS) {
                selection_list_try_move_index(packets, -1);
                break;
            }
        } else if (currentIndexPTS == targetPTS) {
            break;
        }

        if (!result) {
            break;
        }

        current = (AVPacket*)selection_list_get(packets);
        currentIndexPTS = current->pts;

        if (i64min(currentIndexPTS, lastPTS) <= targetPTS && i64max(currentIndexPTS, lastPTS) >= targetPTS) {
            break;
        }

        lastPTS = currentIndexPTS;
    } 
}
