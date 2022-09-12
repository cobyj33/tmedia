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

    typedef struct ascii_image {
        char lines[MAX_FRAME_HEIGHT + 1][MAX_FRAME_WIDTH + 1];
        int width;
        int height;
    } ascii_image;

    ascii_image copy_ascii_image(ascii_image* src);
    ascii_image get_ascii_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight);
    ascii_image get_ascii_image_bounded(pixel_data* pixelData, int maxWidth, int maxHeight);
    ascii_image get_ascii_image_from_frame(AVFrame* videoFrame, int maxWidth, int maxHeight);
    char get_char_from_value(uint8_t value);
    char get_char_from_area(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
    void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height);
    void get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight, int* width, int* height);
    void overlap_ascii_images(ascii_image* first, ascii_image* second);

