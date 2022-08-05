#pragma once
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

const std::string value_characters = "@\%#*+=-:._";

struct ascii_image {
    std::vector<std::string> lines;
    int width;
    int height;
};

struct pixel_data {
    cv::Vec3b* pixels;
    int width;
    int height;
};
