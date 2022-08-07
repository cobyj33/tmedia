#ifndef MAIN_ASCII_FUNCTIONS_HEADER
    #define MAIN_ASCII_FUNCTIONS_HEADER
        #include <opencv2/opencv.hpp>
        #include <string>
        #include <ascii_data.h>


    // const std::string value_characters = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
    // const std::string value_characters = "█▉▊▋▌▍▎▏";

    // const std::string value_characters = "░▒▓";

    void print_ascii_image(ascii_image* image);
    ascii_image get_image(Color* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight);
    void get_pixels(cv::Mat* image, Color* pixels, int width, int height);
    char get_char_from_values(Color values);
    char get_char_from_area(Color* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
    void get_window_size(int* width, int* height);
    void get_output_size(int srcWidth, int srcHeight, int* width, int* height);
    int get_grayscale(int r, int g, int b);

#endif
