
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
#include <icons.h>
#include <media.h>
#include <threads.h>
#include <wmath.h>
#include <wtime.h>
#include <mutex>
#include <map>

#include <curses_helpers.h>

extern "C" {
#include <ncurses.h>
#include <libavutil/avutil.h>
}

const int KEY_ESCAPE = 27;

const int NUMBER_OF_DISPLAY_MODES = 2;
enum class MediaDisplayMode {
    VIDEO = 0,
    AUDIO = 1
};
const MediaDisplayMode display_modes[NUMBER_OF_DISPLAY_MODES] = { MediaDisplayMode::VIDEO, MediaDisplayMode::AUDIO };

std::string get_media_display_mode_string(MediaDisplayMode currentMode) {
    switch (currentMode) {
        case MediaDisplayMode::VIDEO: return "Video Display Mode";
        case MediaDisplayMode::AUDIO: return "Audio Display Mode";
    }
    throw std::runtime_error("Could not get media display mode string for mode " + std::to_string((int)currentMode));
}

MediaDisplayMode get_next_display_mode(MediaDisplayMode currentMode) {
    for (int i = 0; i < NUMBER_OF_DISPLAY_MODES; i++) {
        if (display_modes[i] == currentMode) {
            return display_modes[(i + 1) % NUMBER_OF_DISPLAY_MODES];
        }
    }
    throw std::runtime_error("Could not get next display mode. display mode " + get_media_display_mode_string(currentMode) );
}


typedef struct AudioGuiData {
    int channel_index;
    bool show_all_channels;
} AudioGuiData;

typedef struct VideoGuiData {
    bool show_color;
    bool fullscreen;
} VideoGuiData;

typedef struct GuiData {
    MediaDisplayMode mode;
    AudioGuiData audio;
    VideoGuiData video;
    bool show_debug;
} GuiData;

void process_render(MediaPlayer* player, std::mutex& alter_mutex, GuiData& gui_data, int input);
void render_screen(MediaPlayer* player, GuiData& gui_data);
void render_movie_screen(MediaPlayer* player, GuiData& gui_data);
void render_video_debug(MediaPlayer* player, GuiData& gui_data);
void render_audio_debug(MediaPlayer* player, GuiData& gui_data);
void render_audio_screen(MediaPlayer* player, GuiData& gui_data);

void print_pixel_data(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight);
void print_pixel_data_grayscale(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight);
void print_pixel_data_colored(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight);

RGBColor get_index_display_color(int index, int length) {
    const double step = (255.0 / 2.0) / length;
    const double color_space_size = 255 - (step * (double)(index / 6));
    int red = index % 6 >= 3 ? color_space_size : 0;
    int green = index % 2 == 0 ? color_space_size : 0;
    int blue = index % 6 < 3 ? color_space_size : 0;
    return RGBColor(red, green, blue);
}

const double TIME_CHANGE_AMOUNT = 10.0;
const double VOLUME_CHANGE_AMOUNT = 0.05;
const double TIME_CHANGE_WAIT_MILLISECONDS = 25;
const double PLAYBACK_SPEED_CHANGE_WAIT_MILLISECONDS = 250;
const double PLAYBACK_SPEED_CHANGE_INTERVAL = 0.25;



void render_loop(MediaPlayer* player, std::mutex& alter_mutex) {
    WINDOW* inputWindow = newwin(0, 0, 1, 1);

    MEVENT mouse_event;
    mousemask(BUTTON1_PRESSED, NULL);
    nodelay(inputWindow, true);
    keypad(inputWindow, true);
    double jump_time_requested = 0;
    GuiData gui_data = { MediaDisplayMode::VIDEO, { false, true }, { player->displaySettings.use_colors, true }, false };

    while (player->inUse) {
        int input = wgetch(inputWindow);
        process_render(player, alter_mutex, gui_data, input);
        sleep_for_ms(5);
    }

    delwin(inputWindow);
}

void process_render(MediaPlayer* player, std::mutex& alter_mutex, GuiData& gui_data, int input) {
    std::lock_guard<std::mutex> scoped_lock(alter_mutex);
    Playback& playback = player->timeline->playback;

    if (!player->inUse) {
        return;
    }

    // if (jump_time_requested != 0 && (ch != KEY_LEFT && ch != KEY_RIGHT)) {
    //     double targetTime = playback.get_time(system_clock_sec()) + jump_time_requested;
    //     if (targetTime >= player->timeline->mediaData->duration) {
    //         player->inUse = false;
    //         break;
    //     }

    //     jump_to_time(player->timeline, targetTime);
    //     // video_symbol_stack_push(player->displayCache->symbol_stack, jump_time_requested < 0 ? get_video_symbol(BACKWARD_ICON) : get_video_symbol(FORWARD_ICON));
    //     jump_time_requested = 0;
    // }

    if (input == ' ') {
        playback.toggle(system_clock_sec());
    }
    
    // if (input == 'd' || input == 'D') {
    //     gui_data.show_debug = !gui_data.show_debug;
    // }
    
    if (input == 'c' || input == 'C') {
        gui_data.mode = get_next_display_mode(gui_data.mode);
    }
    
    // if (input == KEY_LEFT) {
    //     jump_time_requested -= TIME_CHANGE_AMOUNT;
    // }
    
    // if (input == KEY_RIGHT) {

    //     if (gui_data.mode == MediaDisplayMode::VIDEO) {
    //         jump_time_requested += TIME_CHANGE_AMOUNT;
    //     } else if (gui_data.mode == MediaDisplayMode::AUDIO) {

    //         if (gui_data.audio.show_all_channels) {
    //             gui_data.audio.channel_index = 0;
    //             gui_data.audio.show_all_channels = 0;
    //         } else if (gui_data.audio.channel_index == player->displayCache->audio_stream->get_nb_channels() - 1) {
    //             gui_data.audio.show_all_channels = 1;
    //         } else {
    //             gui_data.audio.channel_index++; 
    //         }
    //     }

    // }
    
    if (input == KEY_UP) {
        playback.change_volume(VOLUME_CHANGE_AMOUNT);
        // video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback.get_volume()));
    }
    
    if (input == KEY_DOWN) {
        playback.change_volume(-VOLUME_CHANGE_AMOUNT);
        // video_symbol_stack_push(player->displayCache->symbol_stack,get_symbol_from_volume(playback.get_volume()));
    }
    
    if (input == 'n' || input == 'N') {
        playback.change_speed(PLAYBACK_SPEED_CHANGE_INTERVAL);
    }
    
    if (input == 'm' || input == 'M') {
        playback.change_speed(-PLAYBACK_SPEED_CHANGE_INTERVAL);
    }
    
    if (input == 'x' || input == 'X' || input == KEY_ESCAPE) {
        player->inUse = !player->inUse;
    }
    
    if (input == 'f' || input == 'F') {
        gui_data.video.fullscreen = !gui_data.video.fullscreen;
    }
    
    if (input == KEY_RESIZE) {
        endwin();
        refresh();
    }
    
    // if (input >= '0' && input <= '9') {
    //     int digit = digit_char_to_int(input);
    //     if (gui_data.mode == MediaDisplayMode::VIDEO) {
    //         double time = digit * player->timeline->mediaData->duration / 10;
    //         jump_time_requested = time - player->timeline->playback.get_time(system_clock_sec());
    //     } else if (gui_data.mode == MediaDisplayMode::AUDIO) {
    //         if (digit == 0) {
    //             gui_data.audio.show_all_channels = 1;
    //         } else {
    //             if (player->displayCache->audio_stream->get_nb_channels() > 0) {
    //                 gui_data.audio.show_all_channels = 0;
    //                 gui_data.audio.channel_index = (digit - 1) % player->displayCache->audio_stream->get_nb_channels();
    //             }
    //         }

    //     }
    // }
    
    if (input == KEY_MOUSE) {
        playback.toggle(system_clock_sec());
    }

    render_screen(player, gui_data);
    if (player->timeline->playback.get_time(system_clock_sec()) > player->timeline->mediaData->duration) {
        player->inUse = false;
    }

    refresh();
}

void render_screen(MediaPlayer* player, GuiData& gui_data) {
    render_movie_screen(player, gui_data);
    player->displayCache.last_rendered_image = player->displayCache.image;
}

// void collapse_wave(float* output, int output_sample_count, float* input, int input_sample_count, int nb_channels) {
//     float step = (float)input_sample_count / output_sample_count;
//     float current_step = 0.0f;
//     for (int i = 0; i < output_sample_count; i++) {
//     float average = 0.0f;
//         for (int ichannel = 0; ichannel < nb_channels; ichannel++) {
//            average = 0.0;
//            for (int origi = (int)(current_step); origi < (int)(current_step + step); origi++) {
//                average += input[origi * nb_channels + ichannel];
//            }
//            average /= step;
//            output[i * nb_channels + ichannel] = average;
//         }
//         current_step += step;
//     }
// }

// void expand_wave(float* output, int output_sample_count, float* input, int input_sample_count, int nb_channels) {
//     float stretch = (float)output_sample_count / input_sample_count;
//     float current_step = 0.0f;
//     for (int i = 0; i < output_sample_count; i++) {
//         for (int ch = 0; ch < nb_channels; ch++) {
//             float current_input = input[(int)(current_step * nb_channels) + ch];
//             float last_input = current_step > stretch ? input[(int)((current_step - stretch) * nb_channels) + ch] : 0.0f;
//             float step = (current_input - last_input) / stretch;
//             float current_sample_value = last_input;
//             for (int orig = (int)current_step; orig < (int)(current_step + stretch); orig++) {
//                 output[(i + orig) * nb_channels + ch] = current_sample_value;
//                 current_sample_value += step;
//             }
//         }
//         current_step += stretch;
//     }
// }

// void normalize_wave(std::vector<float> to_normalize, int nb_channels) {
//     float maxes[nb_channels];
//     for (int i = 0; i < nb_channels; i++) {
//         maxes[i] = -(float)INT32_MAX;
//     }

//     for (int i = 0; i < to_normalize.size(); i++) {
//         maxes[i % nb_channels] = to_normalize[i] > maxes[i % nb_channels] ? to_normalize[i] : maxes[i % nb_channels];
//     }

//     for (int i = 0; i < to_normalize.size(); i++) {
//         to_normalize[i] /= maxes[i % nb_channels];
//     }
// }

// void print_wave(int x, int y, int width, int height, float* wave, int nb_samples, int channel_index, int nb_channels, int use_color) {
//     const int midline = y + (height / 2);
//     int color_pair;
//     if (use_color) {
//         RGBColor color = get_index_display_color(channel_index, nb_channels);
//         color_pair = get_closest_ncurses_color_pair(color);
//         attron(COLOR_PAIR(color_pair));
//     }

//     for (int i = channel_index; i < std::min(nb_samples, width) * nb_channels; i += nb_channels) {
//         if (wave[i] == 0.0f) {
//             continue;
//         }

//         for (int top = midline - (int)((double)height / 2 * wave[i]); top != midline; top += signum(wave[i])) {
//             mvaddch(top, x + i / nb_channels, '*');
//             if (top < 0 || top > LINES) { break; }
//         }
//         mvaddch(midline, x + i / nb_channels, '*');
//     }

//     mvprintw(midline, (COLS / 2) - 6, "Channel #%d", channel_index);

//     if (use_color) {
//         attroff(COLOR_PAIR(color_pair));
//     }
// } 

// void render_audio_screen(MediaPlayer *player, GuiData& gui_data) {
//     erase();
//     MediaDisplayCache& cache = player->displayCache;
//     AudioStream& audio_stream = cache.audio_stream;
//     if (!audio_stream.is_initialized()) {
//         printw("%s\n", "CURRENTLY NO AUDIO DATA TO DISPLAY: AUDIO STREAM IS NOT INITIALIZED");
//         return;
//     }

//     const std::size_t shown_sample_size = 8192 * 2;
//     if (!audio_stream.can_read(shown_sample_size)) {
//         printw("%s %ld %s\n", "CURRENTLY NOT ENOUGH AUDIO DATA TO DISPLAY: FOUND ", audio_stream->get_nb_can_read(), " SAMPLES");
//         return;
//     }


//     float last_samples[shown_sample_size * audio_stream.get_nb_channels()];
//     audio_stream.peek_into(shown_sample_size, last_samples);

//     float wave[COLS * audio_stream.get_nb_channels()];
//     for (int i = 0; i < COLS * audio_stream.get_nb_channels(); i++) {
//         wave[i] = 0.0f;
//     }

//     // for (int i = 0; i < shown_sample_size * audio_stream->get_nb_channels(); i++) {
//     //     last_samples[i] = uint8_sample_to_normalized_float(last_samples_compressed[i]);
//     // }

//     if (shown_sample_size < COLS) {
//         expand_wave(wave, COLS, last_samples, shown_sample_size, audio_stream.get_nb_channels());
//     } else if (shown_sample_size > COLS) {
//         collapse_wave(wave, COLS, last_samples, shown_sample_size, audio_stream.get_nb_channels());
//     } else {
//         for (int i = 0; i < COLS * audio_stream.get_nb_channels(); i++) {
//             wave[i] = last_samples[i];
//         } 
//     }
//     normalize_wave(wave, COLS, audio_stream.get_nb_channels());

//     if (gui_data.audio.show_all_channels) {
//         for (int i = 0; i < audio_stream->get_nb_channels(); i++) {
//             print_wave(0, i * ( LINES / audio_stream->get_nb_channels()), COLS, LINES / audio_stream->get_nb_channels(), wave, COLS, i, audio_stream->get_nb_channels(), player->displaySettings->use_colors);
//         }
//     } else {
//         print_wave(0, 0, COLS, LINES, wave, COLS, gui_data.audio.channel_index, audio_stream->get_nb_channels(), player->displaySettings->use_colors);
//     }

//     const char* format = "Press 0 to see all Channels, Otherwise, press a number to see its specific channel: ";
//     mvprintw(0, (COLS / 2) - (strlen(format) + audio_stream->get_nb_channels() * 3 + 6) / 2, "%s%s", format, gui_data.audio.show_all_channels == 1 ? "|All Channels (0)| " : "All Channels (0) ");
//     for (int i = 0; i < audio_stream->get_nb_channels(); i++) {
//         printw(gui_data.audio.channel_index == i && !gui_data.audio.show_all_channels ? "|%d| " : "%d ", i + 1);  
//     }

// }

void render_movie_screen(MediaPlayer* player, GuiData& gui_data) {
    erase();
    print_pixel_data(player->displayCache.image, 0, 0, COLS, LINES);
}

typedef struct ScreenChar {
    ColorChar character;
    int row;
    int col;
} ScreenChar;

void print_pixel_data(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight) {
    if (has_colors()) {
        print_pixel_data_colored(pixelData, boundsRow, boundsCol, boundsWidth, boundsHeight);
    } else {
        print_pixel_data_grayscale(pixelData, boundsRow, boundsCol, boundsWidth, boundsHeight);
    }
}

void print_pixel_data_grayscale(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight) {
    PixelData bounded = pixelData.bound(boundsWidth, boundsHeight);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = (image.get_height() - boundsHeight) / 2;
    int image_start_col = (image.get_width() - boundsWidth) / 2; 

    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            mvaddch(image_start_row + row, image_start_col + col, image.at(row, col).ch);
        } 
    }
}

void print_pixel_data_colored(PixelData& pixelData, int boundsRow, int boundsCol, int boundsWidth, int boundsHeight) {
    if (!has_colors()) {
        throw std::runtime_error("Attempted to print colored text in terminal that does not support color");
    }

    PixelData bounded = pixelData.bound(boundsWidth, boundsHeight);
    const AsciiImage image(bounded, AsciiImage::ASCII_STANDARD_CHAR_MAP);
    int image_start_row = (image.get_height() - boundsHeight) / 2;
    int image_start_col = (image.get_width() - boundsWidth) / 2; 

    std::map<int, std::vector<ScreenChar>> colors;
    std::vector<int> color_pairs;
    for (int row = 0; row < image.get_height(); row++) {
        for (int col = 0; col < image.get_width(); col++) {
            ColorChar color_char = image.at(row, col);
            int color_pair = get_closest_ncurses_color_pair(color_char.color);
            if (colors.count(color_pair) == 0) {
                colors.emplace(color_pair, std::vector<ScreenChar>());
                color_pairs.push_back(color_pair);
            }
            colors.at(color_pair).push_back({color_char, image_start_row + row, image_start_col + col });
        }
    }

    for (int p = 0; p < color_pairs.size(); p++) {
        int pair = color_pairs[p];
        attron(COLOR_PAIR(pair));
        for (int i = 0; i < colors.at(pair).size(); i++) {
            ScreenChar screen_char = colors.at(pair)[i];
            mvaddch(screen_char.row, screen_char.col, screen_char.character.ch);
        }
        attroff(COLOR_PAIR(pair));
    }
}

