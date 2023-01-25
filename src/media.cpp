
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>

#include "icons.h"
#include "playheadlist.hpp"
#include "wtime.h"
#include <media.h>
#include <wmath.h>
#include <except.h>



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
    throw ascii::not_found_error("Cannot get media stream " + std::string(av_get_media_type_string(media_type)) + " from media data, could not be found");
}

bool has_media_stream(MediaData* media_data, enum AVMediaType media_type) {
    return get_media_stream(media_data, media_type) != NULL;
}


void move_packet_list_to_pts(PlayheadList<AVPacket*>* packets, int64_t targetPTS) {
    int result = 1;
    if (packets->is_empty()) {
        return;
    }

    AVPacket* current = packets->get();
    int64_t lastPTS = current->pts;

    while (1) {
        current = packets->get();
        int64_t currentIndexPTS = current->pts;

        if (packets->at_edge()) {
            break;
        } 

        if (currentIndexPTS > targetPTS) {
            packets->step_backward();
            current = packets->get();
            if (current->pts < targetPTS) {
                break;
            }
        } else if (currentIndexPTS < targetPTS) {
            packets->step_forward();
            current = packets->get();
            if (current->pts > targetPTS) {
                packets->step_backward();
                break;
            }
        } else if (currentIndexPTS == targetPTS) {
            break;
        }

        // if (!result) {
        //     break;
        // }

        current = packets->get();
        currentIndexPTS = current->pts;

        if (std::min(currentIndexPTS, lastPTS) <= targetPTS && std::max(currentIndexPTS, lastPTS) >= targetPTS) {
            break;
        }

        lastPTS = currentIndexPTS;
    }
}

void move_frame_list_to_pts(PlayheadList<AVFrame*>* frames, int64_t targetPTS) {
    int result = 1;
    if (frames->is_empty()) {
        return;
    }

    AVFrame* current = frames->get();
    int64_t lastPTS = current->pts;

    while (1) {
        current = frames->get();
        int64_t currentIndexPTS = current->pts;

        if (frames->at_edge()) {
            break;
        } 

        if (currentIndexPTS > targetPTS) {
            frames->step_backward();
            current = frames->get();
            if (current->pts < targetPTS) {
                break;
            }
        } else if (currentIndexPTS < targetPTS) {
            frames->step_forward();
            current = frames->get();
            if (current->pts > targetPTS) {
                frames->step_backward();
                break;
            }
        } else if (currentIndexPTS == targetPTS) {
            break;
        }

        // if (!result) {
        //     break;
        // }

        current = frames->get();
        currentIndexPTS = current->pts;

        if (std::min(currentIndexPTS, lastPTS) <= targetPTS && std::max(currentIndexPTS, lastPTS) >= targetPTS) {
            break;
        }

        lastPTS = currentIndexPTS;
    }
}

