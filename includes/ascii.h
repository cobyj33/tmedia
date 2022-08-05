#ifndef MAIN_ASCII_FUNCTIONS_HEADER
    #define MAIN_ASCII_FUNCTIONS_HEADER
        #include <opencv2/opencv.hpp>
        #include <string>

    const std::string value_characters = "@\%#*+=-:._";

    // const std::string value_characters = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
    // const std::string value_characters = "█▉▊▋▌▍▎▏";

    struct ascii_image {
        std::vector<std::string> lines;
        int width;
        int height;
    };

    void print_image(cv::Mat* image);
    ascii_image get_image(cv::Mat* target, int outputWidth, int outputHeight);
    void get_pixels(cv::Mat* image, cv::Vec3b* pixels, int width, int height);
    char get_char_from_values(cv::Vec3b values);
    char get_char_from_area(cv::Vec3b* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
    void get_window_size(int* width, int* height);
    void get_output_size(cv::Mat* target, int* width, int* height);
    int get_grayscale(int r, int g, int b);

#endif