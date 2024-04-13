#include <tmedia/tmcurses/tmcurses.h>

#define TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#include <tmedia/tmcurses/internal/tmcurses_internal.h>
#undef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION

bool ncurses_initialized = false;

void ncurses_init() {
  if (ncurses_initialized) return;
  ncurses_initialized = true;
  initscr();
  savetty();
  ncurses_init_color();
  ncurses_set_color_palette(TMNCursesColorPalette::RGB);
  cbreak();
  noecho();
  nodelay(stdscr, true);
  timeout(5);
  keypad(stdscr, true);
  intrflush(stdscr, false); // maybe set to true?
  idcok(stdscr, true);
  idlok(stdscr, true);
  leaveok(stdscr, true);
  nonl();
  curs_set(0);
}

bool ncurses_is_initialized() {
  return ncurses_initialized;
}

void ncurses_uninit() {
  if (!ncurses_initialized) return;
  ncurses_initialized = false;
  erase();
  refresh();
  ncurses_uninit_color();
  keypad(stdscr, false);
  nocbreak();
  echo();
  resetty();
  endwin();
}
