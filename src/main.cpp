#include <modes.h>
#include <ncurses.h>

int main(int argc, char** argv)
{
  initscr();
  return videoProgram(argc, argv);
}