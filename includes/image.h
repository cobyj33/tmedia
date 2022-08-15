#ifndef ASCII_IMAGE_PROGRAM
#define ASCII_IMAGE_PROGRAM

#include <ascii_data.h>

int imageProgram(const char* fileName);
pixel_data get_pixel_data_from_image(const char* fileName, int desiredWidth, int desiredHeight, bool* success);

#endif