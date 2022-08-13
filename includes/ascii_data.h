#pragma once
#include <sys/types.h>
#include <vector>
#include <string>
#include <ascii_constants.h>


const std::string value_characters = "@\%#*+=-:._";

/* typedef struct Color { */
/*     int r; */
/*     int g; */
/*     int b; */
/* } Color; */

typedef struct ascii_image {
    char lines[MAX_FRAME_HEIGHT + 1][MAX_FRAME_WIDTH + 1];
    int width;
    int height;
} ascii_image;

typedef struct pixel_data {
    u_int8_t* pixels;
    int width;
    int height;
} pixel_data;
