#ifndef ASCII_VIDEO_ASCII
#define ASCII_VIDEO_ASCII
#include <cstdint>
#include <vector>
#include <string>
#include "scale.h"
#include "color.h"
#include "pixeldata.h"

class ColorChar {
  public:
    const char ch;
    const RGBColor color;
    ColorChar(char ch, RGBColor& color) : ch(ch), color(color) {}
    ColorChar(char ch, const RGBColor& color) : ch(ch), color(color) {}
    ColorChar(const ColorChar& color_char) : ch(color_char.ch), color(color_char.color) {}
    bool equals(const ColorChar& color_char) const; 
};

class AsciiImage {
  private:
    std::vector<ColorChar> chars;
    int m_width;
    int m_height;

  public:

    static std::string ASCII_STANDARD_CHAR_MAP;
    static std::string ASCII_EXTENDED_CHAR_MAP;
    static std::string UNICODE_BOX_CHAR_MAP;
    static std::string UNICODE_STIPPLE_CHAR_MAP;

    int get_width() const;
    int get_height() const;

    AsciiImage(): chars(std::vector<ColorChar>()), m_width(0), m_height(0) {}
    AsciiImage(const AsciiImage& other);
    AsciiImage(PixelData& pixels, std::string& characters);

    const ColorChar& at(int row, int column) const;
    bool in_bounds(int row, int column) const;
};



char get_char_from_value(std::string& characters, uint8_t value);
char get_char_from_rgb(std::string& characters, RGBColor& color);
char get_char_from_area_rgb(std::string& characters, uint8_t* pixels, int row, int col, int width, int height, int pixelWidth, int pixelHeight);

RGBColor get_rgb_from_char(std::string& characters, char ch);
#endif
