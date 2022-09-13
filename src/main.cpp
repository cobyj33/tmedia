#include "icons.h"
#include <image.h>
#include <video.h>
#include <media.h>
#include <info.h>

extern "C" {
#include <libavutil/log.h>
}

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <ncurses.h>

#include <chrono>

bool isValidPath(const char* path) {
    FILE* file;
    file = fopen(path, "r");

    if (file != NULL) {
        return true;
    }
    return false;
}

const char* help_text = "     ASCII_VIDEO         \n"
      "  -h => help                   \n"
      "  -i <file> => show image file                   \n"
      "  -v <file> => play video file                   \n"
      "       --VIDEO CONTROLS--                   \n"
      "         RIGHT-ARROW -> Forward 10 seconds                   \n"
      "         LEFT-ARROW -> Backward 10 seconds                   \n"
      "         UP-ARROW -> Volume Up 5%                   \n"
      "         DOWN-ARROW -> Volume Down 5%                   \n"
      "         SPACEBAR -> Pause / Play                   \n"
      "         d or D -> Debug Mode                   \n"
      "       ------------------                   \n"
      "  -info <file> => print file info                   \n";


void ncurses_init() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
}

int main(int argc, char** argv)
{
    av_log_set_level(AV_LOG_QUIET);
    init_icons();

    /* return testIconProgram(); */

  if (argc == 1) {
    std::cout << help_text << std::endl;
  } else if (argc == 2) {
    std::cout << argv[1] << std::endl;
    std::cout << help_text << std::endl;

  } else if (argc == 3) {
      if (strcmp(argv[1], "-i") == 0) {
          if (isValidPath(argv[2])) {
              ncurses_init();
                imageProgram(argv[2]); 
          } else {
              std::cout << "Invalid Path: " << argv[2] << std::endl;
          }
      } else if (strcmp(argv[1], "-v") == 0) {
          if (isValidPath(argv[2])) {
              ncurses_init();
            start_media_player_from_filename(argv[2]); 
          } else {
              std::cout << "Invalid Path " << argv[2] << std::endl;
          }
      } else if (strcmp(argv[1], "-info") == 0) {
        if (isValidPath(argv[2])) {
            fileInfoProgram(argv[2]); 
        } else {
            std::cout << "Invalid Path " << argv[2] << std::endl;
        }
      }
      /* else if (strcmp(argv[1], "-ui") == 0) { //TODO: add url validation */
      /*   imageProgram(argv[2]); */
      /* } else if (strcmp(argv[1], "-uv") == 0) { */
      /*   videoProgram(argv[2]); */
      /* } */
      else {
        std::cout << help_text << std::endl;
      }

  } else {
      std::cout << "Too many commands: " << argc << std::endl;
      for (int i = 0; i < argc; i++) {
        std::cout << argv[i] << ", ";
      }
      std::cout << '\n' << std::endl;
      std::cout << help_text << std::endl;
  }

      endwin();
    free_icons();
  return EXIT_SUCCESS;

}
