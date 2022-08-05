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

  print_image(&image);
  refresh();
  char input[1000];
  std::cin >> input;

  return 0;
}
