#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "color.h"
#include "icons.h"
#include "pixeldata.h"
#include "image.h"
#include "video.h"
#include "media.h"
#include "info.h"

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

// const char* help_text = "     ASCII_VIDEO         \n"
//       "  -h => help                   \n"
//       "  -i <file> => show image file                   \n"
//       "  -v <file> => play video file                   \n"
//       "       --VIDEO CONTROLS--                   \n"
//       "         RIGHT-ARROW -> Forward 10 seconds                   \n"
//       "         LEFT-ARROW -> Backward 10 seconds                   \n"
//       "         UP-ARROW -> Volume Up 5%                   \n"
//       "         DOWN-ARROW -> Volume Down 5%                   \n"
//       "         SPACEBAR -> Pause / Play                   \n"
//       "         c or C -> Switch between video view and Audio View                   \n"
//       "         d or D -> Debug Mode                   \n"
//       "       ------------------                   \n"
//       "  -info <file> => print file info                   \n";
      
const char* help_text = "     ASCII_VIDEO         \n"
    "  -h => help                   \n"
    "  file - play file                   \n"
    "   --VIDEO CONTROLS--                   \n"
    "         UP-ARROW -> Volume Up 5%                   \n"
    "         DOWN-ARROW -> Volume Down 5%                   \n"
    "         SPACEBAR -> Pause / Play                   \n"
    "       ------------------                   \n"
    "  -info <file> => print file info                   \n";

int main(int argc, char** argv)
{
    if (argc == 1) {
        std::cout << help_text << std::endl;
        return EXIT_SUCCESS;
    }
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        std::cout << help_text << std::endl;
        return EXIT_SUCCESS;
    }

    srand(time(nullptr));
    av_log_set_level(AV_LOG_QUIET);
    std::set_terminate(on_terminate);

    if (is_valid_path(argv[1])) {
        ncurses_init();
        MediaPlayer player(argv[1]);
        player.start();
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
    initialize_colors();
    initialize_color_pairs();
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