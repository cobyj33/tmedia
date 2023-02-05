
#include <image.h>
#include <pixeldata.h>
#include <ascii.h>
#include <cstdint>
#include <wmath.h>
#include <vector>
#include <stdexcept>

extern "C" {
#include <libavutil/avutil.h>
}

const ColorChar ColorChar::EMPTY = ColorChar(' ', RGBColor::BLACK);
std::string AsciiImage::ASCII_STANDARD_CHAR_MAP = ASCII_STANDARD_CHAR_MAP;
std::string AsciiImage::ASCII_EXTENDED_CHAR_MAP = ASCII_EXTENDED_CHAR_MAP;
std::string AsciiImage::UNICODE_BOX_CHAR_MAP = UNICODE_BOX_CHAR_MAP;
std::string AsciiImage::UNICODE_STIPPLE_CHAR_MAP = UNICODE_STIPPLE_CHAR_MAP;

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
            RGBColor color = pixels.at(row, col);
            this->lines[row].push_back(ColorChar(get_char_from_value(characters, color.get_grayscale_value()), color));
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

char get_char_from_area(std::string& characters, PixelData* pixels, int x, int y, int width, int height) {
    int grayscale = pixels->get_avg_color_from_area(x, y, width, height).get_grayscale_value();
    return get_char_from_value(characters, (uint8_t)grayscale);
}

double get_scale_factor(int srcWidth, int srcHeight, int targetWidth, int targetHeight) {
    if (srcWidth == targetWidth && srcHeight == targetHeight) {
        return 1;
    }

    bool shouldShrink = srcWidth > targetWidth || srcHeight > targetHeight;
    double scaleFactor = shouldShrink ? std::min((double)targetWidth / srcWidth, (double)targetHeight / srcHeight) : std::max((double)targetWidth / srcWidth, (double)targetHeight / srcHeight);
    return scaleFactor;
}

std::pair<int, int> get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight) { 
    double scale_factor = get_scale_factor(srcWidth, srcHeight, targetWidth, targetHeight);
    int width = (int)(srcWidth * scale_factor);
    int height = (int)(srcHeight * scale_factor);
    return std::make_pair(width, height);
}

std::pair<int, int> get_bounded_dimensions(int srcWidth, int srcHeight, int maxWidth, int maxHeight) {
  if (srcWidth <= maxWidth && srcHeight <= maxHeight) {
    return std::make_pair(srcWidth, srcHeight);
  } else {
    return get_scale_size(srcWidth, srcHeight, maxWidth, maxHeight);
  }
}