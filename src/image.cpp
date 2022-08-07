#include <modes.h>
#include <ascii.h>
#include <ncurses.h>
#include <iostream>


int imageProgram(const char* fileName) {
  cv::Mat image;

  image = cv::imread(fileName);

  if (image.empty())
  {
   std::cout << "Could not open or find the image: " << fileName << std::endl;
   std::cin.get();
   return -1;
  }

  initscr();

  Color pixels[image.rows * image.cols];
  get_pixels(&image, pixels, image.cols, image.rows);
  int outputWidth, outputHeight;
  get_output_size(image.cols, image.rows, &outputWidth, &outputHeight);
  ascii_image asciiImage = get_image(pixels, image.cols, image.rows, outputWidth, outputHeight);
  print_ascii_image(&asciiImage);
  refresh();
  char input[1000];
  std::cin >> input;

  return 0;
}
