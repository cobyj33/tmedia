#include "color.h"
#include "macros.h"
#include "pixeldata.h"
#include <image.h>
#include <ascii.h>
#include <renderer.h>
#include <icons.h>
#include <media.h>
#include <thread>
#include <chrono>
#include <mutex>

#include <ncurses.h>
#include <curses_helpers.h>

void render_thread(MediaPlayer* player, std::mutex* alterMutex) {
    std::unique_lock<std::mutex> alterLock(*alterMutex, std::defer_lock);
    int frames_rendered = 0;

    while (player->inUse) {
        alterLock.lock();
        frames_rendered++;
        add_debug_message(player->displayCache->debug_info, "Frames Rendered: %d", frames_rendered);
        render_screen(player);
        alterLock.unlock();
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void render_screen(MediaPlayer* player) {
    if (player->displayCache->image != nullptr && player->displayCache->last_rendered_image != nullptr) {
        if (!pixel_data_equals(player->displayCache->image, player->displayCache->last_rendered_image)) {
            render_movie_screen(player);
        }
    }

    if (player->displayCache->last_rendered_image != nullptr) {
        pixel_data_free(player->displayCache->last_rendered_image);
    }
    if (player->displayCache->image != nullptr) {
        player->displayCache->last_rendered_image = copy_pixel_data(player->displayCache->image);
    }
    /* render_video_debug(player); */
    /* if (player->displaySettings->show_debug) { */
    /*     if (player->displaySettings->mode == VIDEO) { */
    /*         render_video_debug(player); */
    /*     } else if (player->displaySettings->mode == AUDIO) { */
    /*         render_audio_debug(player); */
    /*     } */
    /* } else { */
    /*     if (player->displaySettings->mode == VIDEO) { */
    /*         render_movie_screen(player); */
    /*     } else if (player->displaySettings->mode == AUDIO) { */
    /*         render_audio_screen(player); */
    /*     } */
    /* } */
}

void render_audio_screen(MediaPlayer *player) {
    erase();
    printw("AUDIO");
}

void render_audio_debug(MediaPlayer* player) {
    erase();
    for (int i = 0; i < player->displayCache->debug_info->nb_messages; i++) {
        printw("%s", player->displayCache->debug_info->messages[i]);
    }
    clear_media_debug(player->displayCache->debug_info);
}

void render_video_debug(MediaPlayer *player) {
    erase();
    for (int i = 0; i < player->displayCache->debug_info->nb_messages; i++) {
        printw("%s", player->displayCache->debug_info->messages[i]);
    }

    clear_media_debug(player->displayCache->debug_info);
}

void render_movie_screen(MediaPlayer* player) {
    erase();
    if (player->displayCache->image != nullptr) {
        VideoSymbolStack* symbol_stack = player->displayCache->symbol_stack;
        AsciiImage textImage = get_ascii_image_bounded(player->displayCache->image, COLS, LINES);

        while (player->displayCache->symbol_stack->top >= 0) {
            VideoSymbol* currentSymbol = video_symbol_stack_peek(symbol_stack); 
            if (std::chrono::steady_clock::now() - currentSymbol->startTime < currentSymbol->lifeTime) {
                int currentSymbolFrame = get_video_symbol_current_frame(currentSymbol); 
                AsciiImage symbolImage = get_ascii_image_bounded(currentSymbol->frameData[currentSymbolFrame], textImage.width, textImage.height);
                overlap_ascii_images(&textImage, &symbolImage);
                break;
            } else {
                video_symbol_stack_erase_pop(symbol_stack);
            }
        }

        if (!player->timeline->playback->playing) {
            VideoSymbol* pauseSymbol = get_video_symbol(PAUSE_ICON);
            AsciiImage symbolImage = get_ascii_image_bounded(pauseSymbol->frameData[0], textImage.width, textImage.height);
            overlap_ascii_images(&textImage, &symbolImage);
            free_video_symbol(pauseSymbol);
        }

        /* int nb_quantized = 32; */
        /* rgb output[nb_quantized]; */
        /* rgb* color_data[textImage.height]; */
        /* for (int i = 0; i < textImage.height; i++) { */
        /*     color_data[i] = textImage.color_data[i]; */
        /* } */

        /* quantize_image(output, nb_quantized, color_data, textImage.width, textImage.height); */
        /* rgb reinitialized[nb_quantized]; */
        /* int nb_to_reinit = 0; */
        /* rgb black = { 0, 0, 0 }; */
        /* for (int i = 0; i < nb_quantized; i++) { */
        /*     if (!rgb_equals(output[i], black)) { */
        /*         rgb_copy(reinitialized[nb_to_reinit], output[i]); */
        /*         nb_to_reinit++; */
        /*     } */
        /* } */

        /* initialize_new_colors(reinitialized, nb_to_reinit); */
        print_ascii_image_full(&textImage);
    }
}

void fill_with_char(char* str, int len, char ch) {
    for (int i = 0; i < len; i++) {
        str[i] = ch;
    }
}

typedef struct ScreenChar {
    char character;
    int row;
    int col;
} ScreenChar;

void print_ascii_image_full(AsciiImage* textImage) {
    move(0, 0);
    int horizontalPaddingWidth = (COLS - textImage->width) / 2; 
    int verticalPaddingHeight = (LINES - textImage->height) / 2;
    char horizontalPadding[horizontalPaddingWidth + 1];
    horizontalPadding[horizontalPaddingWidth] = '\0';
    char lineAcross[COLS + 1];
    lineAcross[COLS] = '\0';

    fill_with_char(horizontalPadding, horizontalPaddingWidth, ' ');
    fill_with_char(lineAcross, COLS, ' ');

    for (int i = 0; i < verticalPaddingHeight; i++) {
        printw("%s\n", lineAcross);
    }

    const int max_color_pairs = std::min(COLORS - 8, COLOR_PAIRS);

    if (textImage->colored && max_color_pairs > 16) {
        ScreenChar* char_buckets[COLOR_PAIRS];
        int char_buckets_lengths[COLOR_PAIRS];
        int char_buckets_capacities[COLOR_PAIRS];
        for (int i = 0; i < COLOR_PAIRS; i++) {
            char_buckets[i] = nullptr;
            char_buckets_lengths[i] = 0;
            char_buckets_capacities[i] = 0;
        }

        for (int row = 0; row < textImage->height; row++) {
            for (int col = 0; col < textImage->width; col++) {
                int target_pair = get_closest_color_pair(textImage->color_data[row][col]);
                if (char_buckets[target_pair] == nullptr) {
                    char_buckets[target_pair] = (ScreenChar*)malloc(sizeof(ScreenChar) * 10);
                    if (char_buckets[target_pair] == NULL) {
                        char_buckets[target_pair] = nullptr;
                        continue;
                    }
                    char_buckets_capacities[target_pair] = 10;
                }

                if (char_buckets_lengths[target_pair] >= char_buckets_capacities[target_pair]) {
                    ScreenChar* tmp = (ScreenChar*)realloc(char_buckets[target_pair], sizeof(ScreenChar) * char_buckets_capacities[target_pair] * 2);
                    if (tmp != NULL) {
                        char_buckets[target_pair] = tmp;
                        char_buckets_capacities[target_pair] *= 2;
                    }
                }

                if (char_buckets_lengths[target_pair] < char_buckets_capacities[target_pair]) {
                    char_buckets[target_pair][char_buckets_lengths[target_pair]] = { textImage->lines[row][col], row, col };
                    char_buckets_lengths[target_pair] += 1;
                }
            }
        }

        for (int b = 0; b < COLOR_PAIRS; b++) {
            if (char_buckets[b] == nullptr || char_buckets[b] == NULL) {
                continue;
            }

            attron(COLOR_PAIR(b));
            for (int i = 0; i < char_buckets_lengths[b]; i++) {
                mvaddch(verticalPaddingHeight + char_buckets[b][i].row, horizontalPaddingWidth + char_buckets[b][i].col, char_buckets[b][i].character);
            }

            if (char_buckets[b] != nullptr && char_buckets[b] != NULL) {
                free(char_buckets[b]);
            }
            attroff(COLOR_PAIR(b));
        }
    } else {
        for (int row = 0; row < textImage->height; row++) {
            printw("%s%s%s%s\n", horizontalPadding, "|", textImage->lines[row], "|");
        }
    }

}
