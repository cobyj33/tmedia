
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "audio.h"
#include "color.h"
#include "debug.h"
#include "pixeldata.h"
#include "playheadlist.hpp"
#include <image.h>
#include <video.h>
#include <ascii.h>
#include <renderer.h>
#include <icons.h>
#include <media.h>
#include <threads.h>
#include <wmath.h>
#include <wtime.h>
#include <curses_helpers.h>

extern "C" {
#include <pthread.h>
#include <ncurses.h>
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;

void render_playbar(MediaPlayer* player, GuiData gui_data);

void get_index_display_color(int index, int length, rgb output) {
    const double step = (255.0 / 2.0) / length;
    const double color_space_size = 255 - (step * (double)(index / 6));
    output[0] = index % 6 >= 3 ? color_space_size : 0;
    output[1] = index % 2 == 0 ? color_space_size : 0;
    output[2] = index % 6 < 3 ? color_space_size : 0;
}

int digit_to_int(char num) {
    return num - 48;
}

void render_loop(MediaPlayer* player, pthread_mutex_t* alterMutex) {
    WINDOW* inputWindow = newwin(0, 0, 1, 1);
    MEVENT mouse_event;
    mousemask(BUTTON1_PRESSED, NULL);

    nodelay(inputWindow, true);
    keypad(inputWindow, true);
    double jump_time_requested = 0;
    GuiData gui_data = { DISPLAY_MODE_VIDEO, { 0, 1 }, { player->displaySettings->use_colors, 1 }, 0 };

    const double TIME_CHANGE_AMOUNT = 10.0;
    const double VOLUME_CHANGE_AMOUNT = 0.05;
    const double TIME_CHANGE_WAIT_MILLISECONDS = 25;
    const double PLAYBACK_SPEED_CHANGE_WAIT_MILLISECONDS = 250;
    const double PLAYBACK_SPEED_CHANGE_INTERVAL = 0.25;


    while (player->inUse) {
        pthread_mutex_lock(alterMutex);
        int ch = wgetch(inputWindow);
        Playback* playback = player->timeline->playback;

        if (!player->inUse) {
            pthread_mutex_unlock(alterMutex);
            break;
        }

        if (jump_time_requested != 0 && (ch != KEY_LEFT && ch != KEY_RIGHT)) {
            double targetTime = playback->get_time(system_clock_sec()) + jump_time_requested;
            if (targetTime >= player->timeline->mediaData->duration) {
                player->inUse = 0;
                pthread_mutex_unlock(alterMutex);
                break;
            }

            jump_to_time(player->timeline, targetTime);
            video_symbol_stack_push(player->displayCache->symbol_stack, jump_time_requested < 0 ? get_video_symbol(BACKWARD_ICON) : get_video_symbol(FORWARD_ICON));
            jump_time_requested = 0;
        }

        if (ch == ' ') {
            playback->toggle(system_clock_sec());
        } else if (ch == 'd' || ch == 'D') {
            gui_data.show_debug = !gui_data.show_debug;
        } else if (ch == 'c' || ch == 'C') {
            gui_data.mode = get_next_display_mode(gui_data.mode);
        } else if (ch == KEY_LEFT) {
            jump_time_requested -= TIME_CHANGE_AMOUNT;
        } else if (ch == KEY_RIGHT) {

            if (gui_data.mode == DISPLAY_MODE_VIDEO) {
                jump_time_requested += TIME_CHANGE_AMOUNT;
            } else if (gui_data.mode == DISPLAY_MODE_AUDIO) {

                if (gui_data.audio.show_all_channels) {
                    gui_data.audio.channel_index = 0;
                    gui_data.audio.show_all_channels = 0;
                } else if (gui_data.audio.channel_index == player->displayCache->audio_stream->get_nb_channels() - 1) {
                    gui_data.audio.show_all_channels = 1;
                } else {
                    gui_data.audio.channel_index++; 
                }
            }

        } else if (ch == KEY_UP) {
            playback->change_volume(VOLUME_CHANGE_AMOUNT);
            video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback->get_volume()));
        } else if (ch == KEY_DOWN) {
            playback->change_volume(-VOLUME_CHANGE_AMOUNT);
            video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback->get_volume()));
        } else if (ch == 'n' || ch == 'N') {
            playback->change_speed(PLAYBACK_SPEED_CHANGE_INTERVAL);
        } else if (ch == 'm' || ch == 'M') {
            playback->change_speed(-PLAYBACK_SPEED_CHANGE_INTERVAL);
        } else if (ch == 'x' || ch == 'X' || ch == KEY_ESCAPE) {
            player->inUse = !player->inUse;
        } else if (ch == 'f' || ch == 'F') {
            gui_data.video.fullscreen = !gui_data.video.fullscreen;
        } else if (ch == KEY_RESIZE) {
            endwin();
            refresh();
        } else if (ch >= '0' && ch <= '9') {
            int digit = digit_to_int(ch);
            if (gui_data.mode == DISPLAY_MODE_VIDEO) {
                double time = digit * player->timeline->mediaData->duration / 10;
                jump_time_requested = time - player->timeline->playback->get_time(system_clock_sec());
            } else if (gui_data.mode == DISPLAY_MODE_AUDIO) {
                if (digit == 0) {
                    gui_data.audio.show_all_channels = 1;
                } else {
                    if (player->displayCache->audio_stream->get_nb_channels() > 0) {
                        gui_data.audio.show_all_channels = 0;
                        gui_data.audio.channel_index = (digit - 1) % player->displayCache->audio_stream->get_nb_channels();
                    }
                }

            }
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&mouse_event) == OK) {
                if (mouse_event.bstate & BUTTON1_PRESSED) {
                    if (mouse_event.y >= LINES - 3) {
                        if (player->timeline->mediaData->duration > 0 && COLS > 0) {
                            double time = ((double)mouse_event.x / COLS) * player->timeline->mediaData->duration;
                            jump_time_requested = time - player->timeline->playback->get_time(system_clock_sec());
                        }
                    } else {
                        playback->toggle(system_clock_sec());
                    }
                }
            }

        }

        render_screen(player, gui_data);
        if (player->timeline->playback->get_time(system_clock_sec()) > player->timeline->mediaData->duration) {
            player->inUse = false;
        }

        pthread_mutex_unlock(alterMutex);
        refresh();
        sleep_for_ms(5);
    }

    delwin(inputWindow);
}

void render_screen(MediaPlayer* player, GuiData gui_data) {
    if (gui_data.show_debug) {
        if (gui_data.mode == DISPLAY_MODE_VIDEO) {
            render_video_debug(player, gui_data);
        } else if (gui_data.mode == DISPLAY_MODE_AUDIO) {
            render_audio_debug(player, gui_data);
        }
    } else {
        if (gui_data.mode == DISPLAY_MODE_VIDEO) {
            if (player->displayCache->image != NULL && player->displayCache->last_rendered_image != NULL) {
                if (!pixel_data_equals(player->displayCache->image, player->displayCache->last_rendered_image)) {
                    render_movie_screen(player, gui_data);
                }
            }
        } else if (gui_data.mode == DISPLAY_MODE_AUDIO) {
            render_audio_screen(player, gui_data);
        }
    }


    if (player->displayCache->last_rendered_image != NULL) {
        pixel_data_free(player->displayCache->last_rendered_image);
    }
    if (player->displayCache->image != NULL) {
        player->displayCache->last_rendered_image = copy_pixel_data(player->displayCache->image);
    }
}

void collapse_wave(float* output, int output_sample_count, float* input, int input_sample_count, int nb_channels) {
    float step = (float)input_sample_count / output_sample_count;
    float current_step = 0.0f;
    for (int i = 0; i < output_sample_count; i++) {
    float average = 0.0f;
        for (int ichannel = 0; ichannel < nb_channels; ichannel++) {
           average = 0.0;
           for (int origi = (int)(current_step); origi < (int)(current_step + step); origi++) {
               average += input[origi * nb_channels + ichannel];
           }
           average /= step;
           output[i * nb_channels + ichannel] = average;
        }
        current_step += step;
    }
}

void expand_wave(float* output, int output_sample_count, float* input, int input_sample_count, int nb_channels) {
    float stretch = (float)output_sample_count / input_sample_count;
    float current_step = 0.0f;
    for (int i = 0; i < output_sample_count; i++) {
        for (int ch = 0; ch < nb_channels; ch++) {
            float current_input = input[(int)(current_step * nb_channels) + ch];
            float last_input = current_step > stretch ? input[(int)((current_step - stretch) * nb_channels) + ch] : 0.0f;
            float step = (current_input - last_input) / stretch;
            float current_sample_value = last_input;
            for (int orig = (int)current_step; orig < (int)(current_step + stretch); orig++) {
                output[(i + orig) * nb_channels + ch] = current_sample_value;
                current_sample_value += step;
            }
        }
        current_step += stretch;
    }
}

void normalize_wave(float* to_normalize, int nb_samples, int nb_channels) {
    float maxes[nb_channels];
    for (int i = 0; i < nb_channels; i++) {
        maxes[i] = -(float)INT32_MAX;
    }

    for (int i = 0; i < nb_samples * nb_channels; i++) {
        maxes[i % nb_channels] = to_normalize[i] > maxes[i % nb_channels] ? to_normalize[i] : maxes[i % nb_channels];
    }

    for (int i = 0; i < nb_samples * nb_channels; i++) {
        to_normalize[i] /= maxes[i % nb_channels];
    }
}

void print_wave(int x, int y, int width, int height, float* wave, int nb_samples, int channel_index, int nb_channels, int use_color) {
    const int midline = y + (height / 2);
    int color_pair;
    if (use_color) {
        rgb color;
        get_index_display_color(channel_index, nb_channels, color);
        color_pair = get_closest_color_pair(color);
        attron(COLOR_PAIR(color_pair));
    }

    for (int i = channel_index; i < i32min(nb_samples, width) * nb_channels; i += nb_channels) {
        if (wave[i] == 0.0f) {
            continue;
        }

        for (int top = midline - (int)((double)height / 2 * wave[i]); top != midline; top += fsignum(wave[i])) {
            mvaddch(top, x + i / nb_channels, '*');
            if (top < 0 || top > LINES) { break; }
        }
        mvaddch(midline, x + i / nb_channels, '*');
    }

    mvprintw(midline, (COLS / 2) - 6, "Channel #%d", channel_index);

    if (use_color) {
        attroff(COLOR_PAIR(color_pair));
    }
} 

void render_audio_screen(MediaPlayer *player, GuiData gui_data) {
    erase();
    const MediaDisplayCache* cache = player->displayCache;
    AudioStream* audio_stream = cache->audio_stream;
    if (audio_stream->is_initialized()) {
        printw("%s\n", "CURRENTLY NO AUDIO DATA TO DISPLAY");
        return;
    }

    const int shown_sample_size = 8192 * 2;
    if (audio_stream->can_read(shown_sample_size)) {
        printw("%s\n", "CURRENTLY NOT ENOUGH AUDIO DATA TO DISPLAY");
        return;
    }


    float last_samples[shown_sample_size * audio_stream->get_nb_channels()];
    audio_stream->peek_into(shown_sample_size, last_samples);

    float wave[COLS * audio_stream->get_nb_channels()];
    for (int i = 0; i < COLS * audio_stream->get_nb_channels(); i++) {
        wave[i] = 0.0f;
    }

    // for (int i = 0; i < shown_sample_size * audio_stream->get_nb_channels(); i++) {
    //     last_samples[i] = uint8_sample_to_float(last_samples_compressed[i]);
    // }

    if (shown_sample_size < COLS) {
        expand_wave(wave, COLS, last_samples, shown_sample_size, audio_stream->get_nb_channels());
    } else if (shown_sample_size > COLS) {
        collapse_wave(wave, COLS, last_samples, shown_sample_size, audio_stream->get_nb_channels());
    } else {
        for (int i = 0; i < COLS * audio_stream->get_nb_channels(); i++) {
            wave[i] = last_samples[i];
        } 
    }
    normalize_wave(wave, COLS, audio_stream->get_nb_channels());

    if (gui_data.audio.show_all_channels) {
        for (int i = 0; i < audio_stream->get_nb_channels(); i++) {
            print_wave(0, i * ( LINES / audio_stream->get_nb_channels()), COLS, LINES / audio_stream->get_nb_channels(), wave, COLS, i, audio_stream->get_nb_channels(), player->displaySettings->use_colors);
        }
    } else {
        print_wave(0, 0, COLS, LINES, wave, COLS, gui_data.audio.channel_index, audio_stream->get_nb_channels(), player->displaySettings->use_colors);
    }

    const char* format = "Press 0 to see all Channels, Otherwise, press a number to see its specific channel: ";
    mvprintw(0, (COLS / 2) - (strlen(format) + audio_stream->get_nb_channels() * 3 + 6) / 2, "%s%s", format, gui_data.audio.show_all_channels == 1 ? "|All Channels (0)| " : "All Channels (0) ");
    for (int i = 0; i < audio_stream->get_nb_channels(); i++) {
        printw(gui_data.audio.channel_index == i && !gui_data.audio.show_all_channels ? "|%d| " : "%d ", i + 1);  
    }

}

void print_debug(MediaDebugInfo* debug_info, const char* source, const char* type) {
    for (int i = 0; i < debug_info->nb_messages; i++) {
        if (strcmp(source, debug_info->messages[i]->source) == 0 && strcmp(type, debug_info->messages[i]->type) == 0) {
            printw("%s", debug_info->messages[i]->message);
        }
    }
}

void render_audio_debug(MediaPlayer* player, GuiData gui_data) {
    erase();
    print_debug(player->displayCache->debug_info, "audio", "debug");
    if (player->displayCache->debug_info->nb_messages == MEDIA_DEBUG_MESSAGE_BUFFER_SIZE) {
        clear_media_debug(player->displayCache->debug_info, "audio", "debug");
    }
}

void render_video_debug(MediaPlayer *player, GuiData gui_data) {
    erase();
    print_debug(player->displayCache->debug_info, "video", "debug");
    if (player->displayCache->debug_info->nb_messages == MEDIA_DEBUG_MESSAGE_BUFFER_SIZE) {
        clear_media_debug(player->displayCache->debug_info, "video", "debug");
    }
}

AsciiImage* stitch_video(MediaPlayer* player, int width, int height) {
    VideoSymbolStack* symbol_stack = player->displayCache->symbol_stack;
    AsciiImage* textImage = get_ascii_image_bounded(player->displayCache->image, width, height);
    if (textImage == NULL) {
        return NULL;
    }

    while (player->displayCache->symbol_stack->top >= 0) {
        VideoSymbol* currentSymbol = video_symbol_stack_peek(symbol_stack); 
        if (system_clock_sec() - currentSymbol->startTime < currentSymbol->lifeTime) {
            int currentSymbolFrame = get_video_symbol_current_frame(currentSymbol); 
            AsciiImage* symbolImage = get_ascii_image_bounded(currentSymbol->frameData[currentSymbolFrame], textImage->width, textImage->height);
            if (symbolImage == NULL) {
                free(textImage);
                return NULL;
            }

            if (player->displaySettings->use_colors) {
                ascii_init_color(symbolImage);
            }

            overlap_ascii_images(textImage, symbolImage);
            ascii_image_free(symbolImage);
            break;
        } else {
            video_symbol_stack_erase_pop(symbol_stack);
        }
    }

    if (!player->timeline->playback->is_playing()) {
        VideoSymbol* pauseSymbol = get_video_symbol(PAUSE_ICON);
        AsciiImage* symbolImage = get_ascii_image_bounded(pauseSymbol->frameData[0], textImage->width, textImage->height);
        if (symbolImage == NULL) {
            free(textImage);
            return NULL;
        }

        if (player->displaySettings->use_colors) {
            ascii_init_color(symbolImage);
        }

        overlap_ascii_images(textImage, symbolImage);
        free_video_symbol(pauseSymbol);
        ascii_image_free(symbolImage);
    }

    return textImage;
}

void render_movie_screen(MediaPlayer* player, GuiData gui_data) {
    erase();
    if (player->displayCache->image == NULL) {
        return;
    }
    AsciiImage* textImage = stitch_video(player, COLS, LINES - (gui_data.video.fullscreen ? 0 : 5));
    if (textImage == NULL) {
        return;
    }

    /* const int palette_size = 16; */
    /* int actual_output_size; */
    /* rgb trained_colors[palette_size]; */
    /* get_most_common_colors(trained_colors, palette_size, textImage->color_data, textImage->width * textImage->height, &actual_output_size); */
    /* initialize_new_colors(trained_colors, actual_output_size); */

    /* printw("Actual Output Size: %d\n", actual_output_size); */
    /* for (int i = 0; i < palette_size; i++) { */
    /*     printw("Color #%d: %d %d %d\n", i, trained_colors[i][0], trained_colors[i][1], trained_colors[i][2]); */
    /* } */


    print_ascii_image_full(textImage);
    ascii_image_free(textImage);
    if (!gui_data.video.fullscreen) {
        render_playbar(player, gui_data);
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

    const int max_color_pairs = i32min(COLORS - 8, COLOR_PAIRS);

    if (textImage->colored && max_color_pairs > 16) {
        ScreenChar* char_buckets[COLOR_PAIRS];
        int char_buckets_lengths[COLOR_PAIRS];
        int char_buckets_capacities[COLOR_PAIRS];
        for (int i = 0; i < COLOR_PAIRS; i++) {
            char_buckets[i] = NULL;
            char_buckets_lengths[i] = 0;
            char_buckets_capacities[i] = 0;
        }

        for (int row = 0; row < textImage->height; row++) {
            for (int col = 0; col < textImage->width; col++) {
                int target_pair = get_closest_color_pair(textImage->color_data[row * textImage->width + col]);
                if (char_buckets[target_pair] == NULL) {
                    char_buckets[target_pair] = (ScreenChar*)malloc(sizeof(ScreenChar) * 10);
                    if (char_buckets[target_pair] == NULL) {
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
                    char_buckets[target_pair][char_buckets_lengths[target_pair]] = (ScreenChar){ textImage->lines[row * textImage->width + col], row, col };
                    char_buckets_lengths[target_pair] += 1;
                }
            }
        }

        for (int b = 0; b < COLOR_PAIRS; b++) {
            if (char_buckets[b] == NULL) {
                continue;
            }

            attron(COLOR_PAIR(b));
            for (int i = 0; i < char_buckets_lengths[b]; i++) {
                mvaddch(verticalPaddingHeight + char_buckets[b][i].row, horizontalPaddingWidth + char_buckets[b][i].col, char_buckets[b][i].character);
            }

            if (char_buckets[b] != NULL) {
                free(char_buckets[b]);
            }
            attroff(COLOR_PAIR(b));
        }
    } else {
        for (int row = 0; row < i32min(textImage->height, LINES); row++) {
            printw("%s|", horizontalPadding);
            for (int col = 0; col < i32min(textImage->width, COLS); col++) {
                addch(textImage->lines[row * textImage->width + col]);
            }
            addstr("|\n");
        }
    }
}

int format_time(char* buffer, int buf_size, double time_in_seconds) {
    const long units[4] = { (60 * 60 * 24), (60 * 60), 60, 1 };
    long conversions[4];
    for (int i = 0; i < 4; i++) {
        conversions[i] = time_in_seconds / units[i];
        time_in_seconds -= conversions[i] * units[i];
    }

    return sprintf(buffer, "%ld:%ld:%ld", conversions[1], conversions[2], conversions[3]);
}

void render_playbar(MediaPlayer* player, GuiData gui_data) {
    const double height = fmin(3, fmax(1, LINES * 0.05));
    const double width = COLS;
    werasebox(stdscr, LINES - height, 0, width, height);

    const char* status = player->timeline->playback->is_playing() ? "Playing" : "Paused";
    const double time = player->timeline->playback->get_time(system_clock_sec());
    const double duration = player->timeline->mediaData->duration;

    const int bar_start = strlen(status);
    const int bar_end = COLS;
    mvprintw(LINES - height / 2, 0, "%s", status);
    wfill_box(stdscr, LINES - height / 2, bar_start, bar_end - bar_start, height, '-');
    if (duration != 0.0) {
        wfill_box(stdscr, LINES - height, bar_start, (bar_end - bar_start) * (time / duration), height, '*');
    }
    char playTime[15], durationTime[15];
    format_time(playTime, 15, time);
    format_time(durationTime, 15, duration);
    mvprintw(LINES - height / 2, COLS - (strlen(playTime) + strlen(durationTime) + 1 ), "%s/%s", playTime, durationTime );
}
