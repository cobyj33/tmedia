#ifndef MAIN_ASCII_PROGRAM_HEADER

    #define MAIN_ASCII_PROGRAM_HEADER
    #include <opencv2/opencv.hpp>

    void print_image(cv::Mat* image);
    void get_pixels(cv::Mat* image, cv::Vec3b* pixels, int width, int height);
    char get_char_from_values(cv::Vec3b values);
    char get_char_from_area(cv::Vec3b* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
    void getWindowSize(int* width, int* height);
    void get_output_size(cv::Mat* target, int* width, int* height);

    int imageProgram(int argc, char** argv);
    int videoProgram(int argc, char** argv);
    int average(int argc, int* args);
    int get_grayscale(int r, int g, int b);

#endif