#pragma once
#include <cstdint>
#include <string>
#include "macros.h"
#include "image.h"

extern "C" {
#include <libavutil/frame.h>
} 

// const std::string value_characters = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
// const std::string value_characters = "█▉▊▋▌▍▎▏";

// const std::string value_characters = "░▒▓";


typedef struct ColorChar {
    char ch;
    rgb color;
} ColorChar;

typedef struct AsciiImage {
    char lines[MAX_ASCII_IMAGE_HEIGHT + 1][MAX_ASCII_IMAGE_WIDTH + 1];
    rgb color_data[MAX_ASCII_IMAGE_HEIGHT + 1][MAX_ASCII_IMAGE_WIDTH + 1];
    bool colored;
    int width;
    int height;
} AsciiImage;


AsciiImage copy_ascii_image(AsciiImage* src);
AsciiImage get_ascii_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight, PixelDataFormat format);
AsciiImage get_ascii_image_bounded(PixelData* pixelData, int maxWidth, int maxHeight);
AsciiImage get_ascii_image_from_frame(AVFrame* videoFrame, int maxWidth, int maxHeight);

char get_char_from_value(uint8_t value);
char get_char_from_area(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );

char get_char_from_rgb(rgb values);
char get_char_from_area_rgb(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight);

void get_avg_color_from_area_rgb(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight, rgb output);

void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height);
void get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight, int* width, int* height);
void overlap_ascii_images(AsciiImage* first, AsciiImage* second);

