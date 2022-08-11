#ifndef MAIN_ASCII_FUNCTIONS_HEADER
    #define MAIN_ASCII_FUNCTIONS_HEADER
        #include <cstdint>
#include <string>
        #include "ascii_data.h"


    // const std::string value_characters = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
    // const std::string value_characters = "█▉▊▋▌▍▎▏";

    // const std::string value_characters = "░▒▓";


    void print_ascii_image(ascii_image* image);
    ascii_image get_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight);
    char get_char_from_value(uint8_t value);
    char get_char_from_area(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
    void get_window_size(int* width, int* height);
    void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height);

#endif
