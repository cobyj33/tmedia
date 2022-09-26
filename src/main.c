#include "color.h"
#include "icons.h"
#include "pixeldata.h"
#include <stdlib.h>
#include <image.h>
#include <video.h>
#include <media.h>
#include <info.h>

#include <libavutil/log.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>

typedef struct ProgramCommands ProgramCommands;

typedef enum PriorityType {
    PRIORITY_TYPE_HELP, PRIORITY_TYPE_INFORMATION, PRIORITY_TYPE_UNKNOWN
} PriorityType;

typedef enum InputType {
    INPUT_TYPE_VIDEO, INPUT_TYPE_AUDIO, INPUT_TYPE_IMAGE
} InputType;

typedef enum FormatType {
    FORMAT_TYPE_COLORED, FORMAT_TYPE_GRAYSCALE
} FormatType;

struct ProgramCommands {
    FormatType format;
    InputType input;
    const char* file;
    PriorityType priority;
};

bool str_in_list(const char* search, const char** list, int list_len);
bool is_valid_path(const char* path);
void ncurses_init();
int use_program(ProgramCommands* commands);

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

const int nb_input_flags = 6;
const char* input_flags[6] = { "-v", "--video", "-i", "--image", "-a", "--audio" };
InputType flag_to_input_type(const char* flag);

const int nb_format_flags = 2;
const char* format_flags[2] = { "-c", "--color" };
const int nb_valid_flags = nb_input_flags + nb_format_flags;
FormatType flag_to_format_type(const char* flag);

const int nb_priority_flags = 4;
const char* priority_flags[4] = { "-h", "--help", "-info", "--information" };
PriorityType flag_to_priority_type(const char* flag);

int main(int argc, char** argv)
{
    if (argc == 1) {
        printf("%s", help_text);
        return EXIT_SUCCESS;
    }

  srand(time(NULL));
  av_log_set_level(AV_LOG_QUIET);
  /* av_log_set_level(AV_LOG_VERBOSE); */
  init_icons();

  ProgramCommands commands = { FORMAT_TYPE_GRAYSCALE, INPUT_TYPE_VIDEO, NULL, PRIORITY_TYPE_UNKNOWN };
  for (int i = 1; i < argc; i++) {
      if (is_valid_path(argv[i])) {
        commands.file = argv[i];
      } else if (str_in_list(argv[i], format_flags, nb_format_flags)) {
        commands.format = flag_to_format_type(argv[i]); 
      } else if (str_in_list(argv[i], input_flags, nb_input_flags)) {
          commands.input = flag_to_input_type(argv[i]);
      } else if (str_in_list(argv[i], priority_flags, nb_priority_flags)) {
          commands.priority = flag_to_priority_type(argv[i]);
      }

      use_program(&commands);
  }

  endwin();
  free_icons();
  return EXIT_SUCCESS;
}

int parse_priority_commands(ProgramCommands* commands);
int parse_input_commands(ProgramCommands* commands);

int use_program(ProgramCommands* commands) {
    if (commands->priority == PRIORITY_TYPE_HELP) {
        printf("%s", help_text);
        return EXIT_SUCCESS;
    } else if (commands->priority == PRIORITY_TYPE_INFORMATION && commands->file != NULL) {
        return fileInfoProgram(commands->file);
    } else if (commands->file != NULL) {
        if (commands->input == INPUT_TYPE_IMAGE) {
            ncurses_init();
            return imageProgram(commands->file, has_colors() && commands->format == FORMAT_TYPE_COLORED ? true : false);
        } else if (commands->input == INPUT_TYPE_VIDEO) {
            ncurses_init();
            MediaPlayer* player = media_player_alloc(commands->file);
            player->displaySettings->use_colors = commands->format == FORMAT_TYPE_COLORED && player->displaySettings->can_use_colors;
            if (player != NULL) {
                start_media_player(player);
                media_player_free(player);
                return EXIT_SUCCESS;
            }
            return EXIT_FAILURE;
        } else if (commands->input == INPUT_TYPE_AUDIO) {
            printf("%s", "Audio Only Currently Unimplemented");
            return EXIT_SUCCESS;
        }
    }

    return EXIT_FAILURE;
}

bool str_in_list(const char* search, const char** list, int list_len) {
    for (int i = 0; i < list_len; i++) {
        if (strcmp(list[i], search)) {
            return true;
        }
    }
    return false;
}

bool is_valid_path(const char* path) {
    FILE* file;
    file = fopen(path, "r");

    if (file != NULL) {
        return true;
    }
    return false;
}

void ncurses_init() {
    initscr();
    start_color();
    initialize_colors();
    initialize_color_pairs();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
}

InputType flag_to_input_type(const char* flag) {
    if (strcmp(flag, "-i") == 0 || strcmp(flag, "--input") == 0) {
        return INPUT_TYPE_IMAGE;
    } else if (strcmp(flag, "-i") == 0 || strcmp(flag, "--input") == 0) {
        return INPUT_TYPE_AUDIO;
    }
    return INPUT_TYPE_VIDEO;
}

FormatType flag_to_format_type(const char* flag) {
    if (strcmp(flag, "-c") == 0 || strcmp(flag, "--color") == 0) {
        return FORMAT_TYPE_COLORED;
    } 
    return FORMAT_TYPE_GRAYSCALE;
}

PriorityType flag_to_priority_type(const char* flag) {
    if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0) {
        return PRIORITY_TYPE_HELP;
    } else if (strcmp(flag, "-info") == 0 || strcmp(flag, "--information") == 0) {
        return PRIORITY_TYPE_INFORMATION;
    }
    return PRIORITY_TYPE_UNKNOWN;
}
