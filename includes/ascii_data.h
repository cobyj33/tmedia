#pragma once
#include <vector>
#include <string>
#include <ascii_constants.h>

const std::string value_characters = "@$\%#O^*+o=-:._ ";

typedef struct ascii_image {
    char lines[MAX_FRAME_HEIGHT + 1][MAX_FRAME_WIDTH + 1];
    int width;
    int height;
} ascii_image;

typedef struct pixel_data {
    uint8_t* pixels;
    int width;
    int height;
} pixel_data;

pixel_data* pixel_data_alloc(int width, int height);
void pixel_data_free(pixel_data* pixel_data);
