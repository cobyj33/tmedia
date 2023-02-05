#ifndef ASCII_VIDEO_ASCII
#define ASCII_VIDEO_ASCII
#include <cstdint>
#include <vector>
#include <string>
#include "image.h"
#include "color.h"
#include "pixeldata.h"


const std::string ASCII_STANDARD_CHAR_MAP = "@%#*+=-:._ ";
const std::string ASCII_EXTENDED_CHAR_MAP = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
const std::string UNICODE_BOX_CHAR_MAP = "█▉▊▋▌▍▎▏";
const std::string UNICODE_STIPPLE_CHAR_MAP = "░▒▓";


class ColorChar {
    public:
        static const ColorChar EMPTY;
        const char ch;
        RGBColor color;
        ColorChar(char ch, RGBColor color) : ch(ch), color(color) {}
        ColorChar(const ColorChar& color_char) : ch(color_char.ch), color(color_char.color) {}
        bool equals(ColorChar& color_char) const; 
};

class AsciiImage {
    private:
        std::vector<std::vector<ColorChar>> lines;

    public:

        static std::string ASCII_STANDARD_CHAR_MAP;
        static std::string ASCII_EXTENDED_CHAR_MAP;
        static std::string UNICODE_BOX_CHAR_MAP;
        static std::string UNICODE_STIPPLE_CHAR_MAP;

        int get_width() const;
        int get_height() const;

        AsciiImage(int width, int height);
        AsciiImage(const AsciiImage& other);
        AsciiImage(PixelData& pixels, std::string& characters);

        ColorChar at(int row, int column) const;
        bool in_bounds(int row, int column) const;
        std::vector<ColorChar> get_line(int row) const;
};



char get_char_from_value(std::string& characters, uint8_t value);
char get_char_from_rgb(std::string& characters, RGBColor& color);
char get_char_from_area_rgb(std::string& characters, uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight);

RGBColor get_rgb_from_char(std::string& characters, char ch);
#endif
