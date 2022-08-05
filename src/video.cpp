#include <modes.h>
#include <ascii.h>
#include <ncurses.h>

int videoProgram(int argc, char** argv) {
  cv::VideoCapture capture;

  capture.open("/home/cobyj33/One Piece Dub Episode 1.mp4");

  if (!capture.isOpened()) {
    addstr("Could not open video");
    refresh();
    return EXIT_FAILURE;
  }

  while (true) {
    cv::Mat frame;
    capture >> frame;
    if (frame.empty()) {
      break;
    }

    erase();
    print_image(&frame);
    refresh();
  }

  capture.release();
  return EXIT_SUCCESS;
}