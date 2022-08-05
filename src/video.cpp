#include <modes.h>
#include <ascii.h>
#include <ascii_data.h>
#include <ncurses.h>

#include <thread>
#include <chrono>
#include <iostream>

const int secondToNanosecond = 1000000000;

void frameGenerator(cv::VideoCapture* videoCapture, pixel_data* buffer, int bufferLength) {
    cv::VideoCapture capture = *videoCapture;

    for (int i = 0; i < bufferLength; i++) {
        cv::Mat image;
        capture >> image;
        cv::Vec3b pixels[image.rows * image.cols];
        get_pixels(&image, pixels, image.cols, image.rows);
        
        pixel_data data;
        data.pixels = pixels;
        data.width = image.cols;
        data.height = image.rows;
        buffer[i] = data;
    }

}

int videoProgram(const char* fileName) {
  cv::VideoCapture capture;
    capture.open(fileName);
  
  if (!capture.isOpened()) {
      std::cout << "Could not open file: " << fileName << std::endl;
    return EXIT_FAILURE;
  }

  initscr();

  int totalFrames = capture.get(cv::CAP_PROP_FRAME_COUNT);
  pixel_data buffer[totalFrames];
  int targetFPS = capture.get(cv::CAP_PROP_FPS);
  targetFPS = 24;

  int currentFrame = 0;
  auto start_time = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now());
  auto preferredTime = std::chrono::nanoseconds(0);
  auto actualTime = std::chrono::nanoseconds(0);
  const auto preferredTimePerFrame = std::chrono::nanoseconds((int)(secondToNanosecond / targetFPS));
  while (true) {
    auto diff = actualTime - preferredTime;
    cv::Mat frame;
    capture >> frame;
    if (frame.empty()) {
      break;
    }
    print_image(&frame);
    
    std::this_thread::sleep_for(preferredTimePerFrame - diff);
    preferredTime = std::chrono::nanoseconds(  preferredTime.count() + preferredTimePerFrame.count() );
    actualTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()) - start_time;
    refresh();
    currentFrame++;
  }

  capture.release();
  return EXIT_SUCCESS;
}
