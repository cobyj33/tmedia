#include <modes.h>
#include <ascii.h>
#include <ncurses.h>

int imageProgram(int argc, char** argv) {
  cv::Mat image;

  image = cv::imread("/home/cobyj33/Pictures/frame.jpg");

  if (image.empty())
  {
   std::cout << "Could not open or find the image" << std::endl;
   std::cin.get();
   return -1;
  }

  print_image(&image);
  refresh();
  char input[1000];
  std::cin >> input;

  return 0;
}