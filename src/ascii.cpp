
#include <cstdint>
#include <vector>
#include <stdexcept>

#include "scale.h"
#include "pixeldata.h"
#include "ascii.h"
#include "wmath.h"

extern "C" {
#include <libavutil/avutil.h>
}

const ColorChar ColorChar::EMPTY = ColorChar(' ', RGBColor::BLACK);
std::string AsciiImage::ASCII_STANDARD_CHAR_MAP = "@%#*+=-:_.";
std::string AsciiImage::ASCII_EXTENDED_CHAR_MAP = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
std::string AsciiImage::UNICODE_BOX_CHAR_MAP = "█▉▊▋▌▍▎▏";
std::string AsciiImage::UNICODE_STIPPLE_CHAR_MAP = "░▒▓";

bool ColorChar::equals(ColorChar& other) const {
    return this->ch == other.ch && this->color.equals(other.color);
}

AsciiImage::AsciiImage(int width, int height) {
    for (int row = 0; row < height; row++) {
        this->lines.push_back(std::vector<ColorChar>());
        for (int column = 0; column < width; column++) {
            this->lines[row].push_back(ColorChar::EMPTY);
        }
    }
}

int AsciiImage::get_width() const {
    if (this->lines.size() == 0) {
        return 0;
    }
    return this->lines[0].size();
}

int AsciiImage::get_height() const {
    if (this->lines.size() == 0) {
        return 0;
    }
    return this->lines.size();
}

AsciiImage::AsciiImage(const AsciiImage& src) {
    for (int row = 0; row < src.get_height(); row++) {
        this->lines.push_back(std::vector<ColorChar>());
        for (int col = 0; col < src.get_width(); col++) {
            this->lines[row].push_back(src.at(row, col));
        }
    }
}

AsciiImage::AsciiImage(PixelData& pixels, std::string& characters) {

    for (int row = 0; row < pixels.get_height(); row++) {
        this->lines.push_back(std::vector<ColorChar>());
        for (int col = 0; col < pixels.get_width(); col++) {
            this->lines[row].push_back(ColorChar(get_char_from_value(characters, pixels.at(row, col).get_grayscale_value()), pixels.at(row, col) ));
        }
    }
    
}

ColorChar AsciiImage::at(int row, int col) const {
    return this->lines[row][col];
}

char get_char_from_value(std::string& characters, uint8_t value) {
  return characters[ (int)value * (characters.length() - 1) / 255 ];
}

RGBColor get_rgb_from_char(std::string& characters, char ch) {
    for (int i = 0; i < characters.size(); i++) {
        if (ch == characters[i]) {
            uint8_t val = i * 255 / (characters.size() - 1); 
            return RGBColor(val);
        }
    }
    
    throw std::runtime_error("Cannot get color value from char " + std::to_string(ch) + ", character \"" + std::to_string(ch) + "\" not found in string: " + characters);
}

char get_char_from_rgb(std::string& characters, RGBColor& color) {
    return get_char_from_value(characters, (uint8_t)get_grayscale(color.red, color.green, color.blue));
}

char get_char_from_area(std::string& characters, PixelData* pixels, int row, int col, int width, int height) {
    int grayscale = pixels->get_avg_color_from_area(row, col, width, height).get_grayscale_value();
    return get_char_from_value(characters, (uint8_t)grayscale);
}

