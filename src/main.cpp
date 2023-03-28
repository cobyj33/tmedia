#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "color.h"
#include "icons.h"
#include "pixeldata.h"
#include "scale.h"
#include "media.h"
#include "formatting.h"

#include "gui.h"
#include <termcolor.h>

#include <argparse.hpp>


extern "C" {
#include "ncurses.h"
#include <libavutil/log.h>
}

#define ASCII_VIDEO_VERSION "0.4.0"

bool ncurses_initialized = false;

bool is_valid_path(const char* path);
void ncurses_init();
void ncurses_uninit();

void on_terminate() {
    if (ncurses_initialized) {
        ncurses_uninit();
    }

    std::exception_ptr exptr = std::current_exception();
    try {
        std::rethrow_exception(exptr);
    }
    catch (std::exception &ex) {
        std::cerr << "Terminated due to exception: " << ex.what() << std::endl;
    }

    std::abort();
}


int main(int argc, char** argv)
{
    srand(time(nullptr));
    av_log_set_level(AV_LOG_QUIET);
    std::set_terminate(on_terminate);

    argparse::ArgumentParser parser("ascii_video", ASCII_VIDEO_VERSION);

    parser.add_argument("file")
        .help("The file to be played");

    const std::string controls = std::string("-------CONTROLS-----------\n") +
                            "SPACE - Toggle Playback \n" +
                            "Left Arrow - Skip backward 5 seconds\n" +
                            "Right Arrow - Skip forward 5 seconds \n" +
                            "Escape or Backspace - End Playback \n"
                            "C - Change to color mode on supported terminals \n"
                            "G - Change to grayscale mode \n"
                            "B - Change to Background Mode on supported terminals if in Color or Grayscale mode \n";

    parser.add_description(controls);

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

    parser.add_argument("-d", "--dump")
        .help("Print metadata about the file")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("-q", "--quiet")
        .help("Silence any warnings outputted before the beginning of playback")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("-t", "--time")
        .help("Choose the time to start media playback")
        .default_value(std::string("00:00"));
    
    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return EXIT_FAILURE;
    }

    std::string file = parser.get<std::string>("file");
    
    double start_time = 0.0;
    bool colors = parser.get<bool>("-c");
    bool grayscale = !colors && parser.get<bool>("-g");
    bool background = parser.get<bool>("-b");
    bool dump = parser.get<bool>("-d");
    bool version = parser.get<bool>("-v");
    bool quiet = parser.get<bool>("-q");
    std::string inputted_start_time = parser.get<std::string>("-t");

    if (version) {
        std::cout << ASCII_VIDEO_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    if (dump) {
        dump_file_info(file.c_str());
        return EXIT_SUCCESS;
    }

    if (!is_valid_path(file.c_str()) && file.length() > 0) {
        std::cerr << "[ascii_video] Cannot open invalid path: " << file << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path path(file);
    std::error_code ec; // For using the non-throwing overloads of functions below.
    if (std::filesystem::is_directory(path, ec))
    { 
        std::cerr << "[ascii_video] Given path " << file << " is a directory." << std::endl;
        return EXIT_FAILURE; 
    }

    if (inputted_start_time.length() > 0) {
        if (is_duration(inputted_start_time)) {
            start_time = parse_duration(inputted_start_time);
        } else if (is_int_str(inputted_start_time)) {
            start_time = std::stoi(inputted_start_time);
        } else {
            std::cerr << "Inputted time must be in seconds, H:MM:SS, or M:SS format (got \"" << inputted_start_time << "\" )" << std::endl;
            return EXIT_FAILURE;
        }
    }

    double file_duration = get_file_duration(file.c_str());
    if (start_time < 0) { // Catch errors in out of bounds times
        std::cerr << "Cannot start file at a negative time ( got time " << double_to_fixed_string(start_time, 2) << "  from input " + inputted_start_time + " )" << std::endl;
        return EXIT_FAILURE;
    } else if (start_time >= file_duration) {
        std::cerr << "Cannot start file at a time greater than duration of media file ( got time " << double_to_fixed_string(start_time, 2) << " seconds from input " << inputted_start_time << ". File ends at " << double_to_fixed_string(file_duration, 2) << " seconds (" << format_duration(file_duration) << ") ) "  << std::endl;
        return EXIT_FAILURE;
    }

    ncurses_init();
    std::string warnings;

    if (colors && grayscale) {
        warnings += "WARNING: Cannot have both colored (-c, --color) and grayscale (-g, --gray) output. Falling back to colored output\n";
        grayscale = false;
    }

    if ( (colors || grayscale) && !has_colors()) {
        warnings += "WARNING: Terminal does not support colored output\n";
        colors = false;
        grayscale = false;
    }

    if (background && !(colors || grayscale) ) {
        if (!has_colors()) {
            warnings += "WARNING: Cannot use background option as colors are not available in this terminal. Falling back to pure text output\n";
            background = false;
        } else {
            warnings += "WARNING: Cannot only show background without either color (-c, --color) or grayscale(-g, --gray, --grayscale) options selected. Falling back to colored output\n";
            colors = true;
            grayscale = false;
        }
    }

    if ( (grayscale || colors) && has_colors() && !can_change_color()) {
        warnings += "WARNING: Terminal does not support changing colored output data, colors may not be as accurate as expected\n";
    }

    if (warnings.length() > 0 && !quiet) {
        printw("%s\n\n%s\n", warnings.c_str(), "Press any key to continue");
        refresh();
        getch();
    }
    erase();

    VideoOutputMode mode = get_video_output_mode_from_params(colors, grayscale, background);

    if (mode == VideoOutputMode::GRAYSCALE || mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY) {
        ncurses_initialize_grayscale_color_palette();
    }
    MediaGUI media_gui;
    media_gui.set_video_output_mode(mode);
    MediaPlayer player(file.c_str(), media_gui);
    player.start(start_time);
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
    ncurses_initialize_color_palette();
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