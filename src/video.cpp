#include <modes.h>
#include <ascii.h>
#include <ncurses.h>

#include <thread>
#include <chrono>
int videoProgram(int argc, char** argv) {
  cv::VideoCapture capture;

  if (argc == 2) {
    capture.open(argv[1]);
  } else {
    capture.open("/home/cobyj33/Fullmetal Alchemist Brotherhood Episode 1.mp4");
  }

  
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
    // print_ascii_image(framebuffer[currentFrame]);
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 24));
    refresh();
  }

  capture.release();
  return EXIT_SUCCESS;
}