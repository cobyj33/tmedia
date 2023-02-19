#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "color.h"
#include "termcolor.h"
#include "icons.h"
#include "pixeldata.h"
#include "scale.h"
#include "media.h"

#include "gui.h"

#include <argparse.hpp>


extern "C" {
#include <ncurses.h>
#include <libavutil/log.h>
}

bool ncurses_initialized = false;

bool is_valid_path(const char* path);
void ncurses_init();
void ncurses_uninit();

void on_terminate() {
    if (ncurses_initialized) {
        ncurses_uninit();
    }

    abort();
}


int main(int argc, char** argv)
{
    srand(time(nullptr));
    av_log_set_level(AV_LOG_QUIET);
    std::set_terminate(on_terminate);

    argparse::ArgumentParser parser("ascii_video", "1.1");

    parser.add_argument("-v", "--video")
        .default_value(true)
        .implicit_value(true)
        .help("Play a video file");
    
    parser.add_argument("-c", "--color")
        .default_value(false)
        .implicit_value(true)
        .help("Play the video with color");

    parser.add_argument("-g", "--grayscale", "--gray")
        .default_value(false)
        .implicit_value(true)
        .help("Play the video in grayscale");

    parser.add_argument("-b", "--background")
        .default_value(false)
        .implicit_value(true)
        .help("Do not show characters, only the background");

    parser.add_argument("file")
        .help("The file to be played");

    parser.add_argument("-d", "--dump")
        .help("Print metadata about the file")
        .default_value(false)
        .implicit_value(true);

    try {
        parser.parse_args(argc, argv);
    }
        catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    std::string file = parser.get<std::string>("file");

    if (is_valid_path(file.c_str())) {
        bool colors = parser.get<bool>("-c");
        bool grayscale = !colors && parser.get<bool>("-g");
        bool background = parser.get<bool>("-b");
        bool dump = parser.get<bool>("-d");

        if (dump) {
            dump_file_info(file.c_str());
            return EXIT_SUCCESS;
        }
        
        ncurses_init();
        std::string warnings;

        if ( (colors || grayscale) && !has_colors()) {
            warnings += "WARNING: Terminal does not support colored output\n";
            colors = false;
            grayscale = false;
        }

        if (background && !(colors || grayscale) ) {
            if (!has_colors()) {
                warnings += "WARNING: Cannot use background option as colors are not available in this terminal\n";
            } else {
                warnings += "WARNING: Cannot only show background without either color (-c, --color) or grayscale(-g, --gray, --grayscale) options selected\n";
            }

            background = false;
        }

        if ( (grayscale || colors) && has_colors() && !can_change_color()) {
            warnings += "WARNING: Terminal does not support changing colored output data, colors may not be as accurate as expected\n";
        }

        if (warnings.length() > 0) {
            printw("%s", warnings.c_str());
            printw("%s\n", "Press any key to continue");
            refresh();
            getch();
        }
        erase();

        VideoOutputMode mode = get_video_output_mode_from_params(colors, grayscale, background);

        if (mode == VideoOutputMode::GRAYSCALE || mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY) {
            ncurses_initialize_grayscale_colors();
        }

        GUIState gui_state;
        gui_state.set_video_output_mode(mode);

        bool video = parser.get<bool>("-v");
        MediaPlayer player(file.c_str());
        player.start(gui_state);
        ncurses_uninit();
    } else {
        throw std::runtime_error("Cannot open invalid path: " + file);
        std::exit(EXIT_FAILURE);
    }

    ncurses_uninit();
    return EXIT_SUCCESS;
}

bool is_valid_path(const char* path) {
    FILE* file;
    file = fopen(path, "r");

    if (file != nullptr) {
        fclose(file);
        return true;
    }

    return false;
}

void ncurses_init() {
    if (ncurses_initialized) {
        throw std::runtime_error("ncurses attempted to be initialized although it has already been initialized");
    }
    ncurses_initialized = true;
    initscr();
    start_color();
    ncurses_initialize_colors();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
}

void ncurses_uninit() {
    if (!ncurses_initialized) {
        throw std::runtime_error("ncurses attempted to be uninitialized although it has never been initialized");
    }
    ncurses_initialized = false;
    endwin();
}