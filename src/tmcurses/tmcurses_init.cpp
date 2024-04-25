#include <tmedia/tmcurses/tmcurses.h>

#define TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION
#include <tmedia/tmcurses/internal/tmcurses_internal.h>
#undef TMEDIA_TMCURSES_INTERNAL_IMPLEMENTATION

bool tmcurses_initialized = false;

void tmcurses_init() {
  if (tmcurses_initialized) return;
  tmcurses_initialized = true;
  initscr();
  savetty();
  tmcurses_init_color();

  tmcurses_set_color_palette(TMNCursesColorPalette::RGB);
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

bool tmcurses_is_initialized() {
  return tmcurses_initialized;
}

void tmcurses_uninit() {
  if (!tmcurses_initialized) return;
  tmcurses_initialized = false;
  erase();
  refresh();
  tmcurses_uninit_color();
  keypad(stdscr, false);
  nocbreak();
  echo();
  resetty();
  endwin();
}
