#pragma once
#include <vector>
#include <string>

const std::string value_characters = "@\%#*+=-:._";

struct ascii_image {
    std::vector<std::string> lines;
    int width;
    int height;
};