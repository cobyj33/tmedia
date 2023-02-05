#ifndef ASCII_VIDEO_COLOR
#define ASCII_VIDEO_COLOR
#include <cstdint>
#include <vector>


// typedef uint8_t rgb[3];
// typedef int rgb_i32[3];

class RGBColor {
    public:
        int red;
        int green;
        int blue;

        static RGBColor BLACK;
        static RGBColor WHITE; 

        RGBColor() : red(0), green(0), blue(0) {}
        RGBColor(int gray) : red(gray), green(gray), blue(gray) {}
        RGBColor(int red, int green, int blue) : red(red), green(green), blue(blue) {}
        RGBColor(const RGBColor& color) : red(color.red), green(color.green), blue(color.blue) {}
        
        double distance(RGBColor& other) const;
        double distance_squared(RGBColor& other) const;
        RGBColor get_complementary() const;
        bool equals(RGBColor& other) const;
        bool is_grayscale() const;
        RGBColor create_grayscale() const;
        int get_grayscale_value() const;
};



int get_grayscale(int r, int g, int b);
int find_closest_color_index(RGBColor& input, std::vector<RGBColor>& colors);
RGBColor find_closest_color(RGBColor& input, std::vector<RGBColor>& colors);
RGBColor get_average_color(std::vector<RGBColor>& colors);



// double color_distance(RGBColor& first, RGBColor& second);
// double color_distance_squared(rgb first, rgb second);
// int get_grayscale_rgb(rgb rgb_color);
// void rgb_set(rgb rgb, uint8_t r, uint8_t g, uint8_t b);
// void rgb_copy(rgb dest, rgb src);
// void rgb_complementary(rgb dest, rgb src);
// int rgb_equals(rgb first, rgb second);
// void rgb_get_values(rgb rgb, uint8_t* r, uint8_t* g, uint8_t* b);

#endif
