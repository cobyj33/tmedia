// inspired by this stackoverflow answer https://stackoverflow.com/a/29556260
#ifndef WRAPPED_NCURSES_H
#define WRAPPED_NCURSES_H


#include <ncurses.h>

namespace ncurses {
    //constants
    using ::COLOR_PAIR;
    using ::COLOR_PAIRS;
    using ::COLORS;

    constexpr int _KEY_RIGHT = KEY_RIGHT;
    constexpr int _KEY_LEFT = KEY_LEFT;




    //functions
    using ::move;
    using ::endwin;
    using ::newwin;
    using ::refresh;
    using ::initscr;
    using ::WINDOW;
    using ::keypad;
    using ::nodelay;
    using ::mousemask;
    using ::cbreak;
    using ::noecho;
    using ::curs_set;

    using ::start_color;
    using ::has_colors;
    using ::can_change_color;


    using ::pair_content;
    using ::color_content;

    using ::init_pair;
    using ::init_color;
    
    using ::mvaddch;
    using ::erase;
    using ::delwin;
    using ::mvprintw;
    using ::printw;







}
#endif