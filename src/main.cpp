#include <image.h>
#include <video.h>
#include <info.h>


#include <stdio.h>
#include <iostream>
#include <string.h>

#include <chrono>

bool isValidPath(const char* path) {
    FILE* file;
    file = fopen(path, "r");

    if (file != NULL) {
        return true;
    }
    return false;
}

const char* help_text = "     ASCII_VIDEO         \n"
      "  -h => help                   \n"
      "  -i <file> => show image file                   \n"
      "  -v <file> => play video file                   \n"
      "       --VIDEO CONTROLS--                   \n"
      "         RIGHT-ARROW -> Forward 5 seconds                   \n"
      "         LEFT-ARROW -> Backward 5 seconds                   \n"
      "         UP-ARROW -> Volume Up 5%                   \n"
      "         DOWN-ARROW -> Volume Down 5%                   \n"
      "         SPACEBAR -> Pause / Play                   \n"
      "         d or D -> Debug Mode                   \n"
      "       ------------------                   \n"
      "  -ui <file> => play image URL                   \n"
      "  -uv <file> => play video URL                   \n"
      "  -info <file> => print file info                   \n";


int main(int argc, char** argv)
{
  if (argc == 1) {
    // testIconProgram();
    // return EXIT_SUCCESS;
    std::cout << help_text << std::endl;
  } else if (argc == 2) {
    std::cout << argv[1] << std::endl;
    std::cout << help_text << std::endl;

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
      } else if (strcmp(argv[1], "-info") == 0) {
        if (isValidPath(argv[2])) {
                fileInfoProgram(argv[2]); 
        } else {
            std::cout << "Invalid Path " << argv[2] << std::endl;
        }
      }
      /* else if (strcmp(argv[1], "-ui") == 0) { //TODO: add url validation */
      /*   imageProgram(argv[2]); */
      /* } else if (strcmp(argv[1], "-uv") == 0) { */
      /*   videoProgram(argv[2]); */
      /* } */
      else {
        std::cout << help_text << std::endl;
      }

  } else {
      std::cout << "Too many commands" << std::endl;
      std::cout << help_text << std::endl;
  }

  return EXIT_SUCCESS;

}
