#include <modes.h>
#include <ncurses.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

bool isValidPath(const char* path) {
    FILE* file;

    if (file = fopen(path, "r")) {
        return true;
    }
    return false;
}

const char* help_text = "     ASCII_VIDEO         \n"
      "  -h => help                   \n"
      "  -i <file> => show image file                   \n"
      "  -v <file> => play video file                   \n"
      "                     \n";

int main(int argc, char** argv)
{

  if (argc == 1) {
    
      std::cout << help_text << std::endl;

  } else if (argc == 2) {
        std::cout << argv[1]; 
      if (strcmp(argv[1], "-i") == 0) {
        std::cout << "Must include relative path to view image" << std::endl;        
      } else if (strcmp(argv[1], "-v") == 0) {
        std::cout << "Must include relative path to view video" << std::endl;        
      } else if (strcmp(argv[1], "-h") == 0) {
          std::cout << help_text << std::endl;
      } else {
          std::cout << help_text << std::endl;
      }


  } else if (argc == 3) {

      if (strcmp(argv[1], "-i") == 0) {
          if (isValidPath(argv[2])) {
                imageProgram(argv[2]); 
          } else {
              std::cout << "Invalid Path: " << argv[2] << std::endl;
          }
      } else if (strcmp(argv[1], "-v") == 0) {
          if (isValidPath(argv[2])) {
                videoProgram(argv[2]); 
          } else {
              std::cout << "Invalid Path " << argv[2] << std::endl;
          }
      } else {
        std::cout << help_text << std::endl;
      }

  } else {
      std::cout << "Too many commands" << std::endl;
      std::cout << help_text << std::endl;
  }

  return EXIT_SUCCESS;

}
