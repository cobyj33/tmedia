#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

const std::string value_characters = "@\%#*+=-:._";

typedef struct Color {
    int r;
    int g;
    int b;
} Color;
typedef struct ascii_image {
    std::vector<std::string> lines;
    int width;
    int height;
} ascii_image;

typedef struct pixel_data {
    Color* pixels;
    int width;
    int height;
} pixel_data;
