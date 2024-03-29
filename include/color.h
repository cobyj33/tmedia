#ifndef TMEDIA_COLOR_H
#define TMEDIA_COLOR_H

#include <vector>
#include <cstddef>

/**
 * @brief A representation of an RGB Color with R, G, and B channels in the
 * range 0-255
 * 
 * Note that while RGBColor assumes that colors are in the range 0-255, colors
 * in different ranges can still be stored inside of a RGBColor object. The user
 * should strongly stick to using color channels in the range 0-255 inclusive though
 * and only convert to different ranges when completely necessary, such as interacting
 * with curses colors in the range 0-1000.
 */
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

    void operator=(const RGBColor& color);
    bool operator==(const RGBColor& color) const;

    double distance(const RGBColor& other) const;
    double distance_squared(const RGBColor& other) const;
    RGBColor get_complementary() const;
    bool equals(const RGBColor& other) const;
    bool is_grayscale() const;
    RGBColor create_grayscale() const;
    int get_grayscale_value() const;
};


class RGBColorHashFunction {
public:
    // Ripped from http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf pg 666 sec 4.1 paragraph 2
    std::size_t operator()(const RGBColor& rgb) const {
      return (73856093UL * static_cast<std::size_t>(rgb.red)) ^
            (19349663UL * static_cast<std::size_t>(rgb.green)) ^
            (83492791UL * static_cast<std::size_t>(rgb.blue));
    }
};

/**
 * @brief Get the grayscale value of 3 channels of rgb data.
 * 
 * @note This function should work for r >= 0, g >= 0, b >= 0, no matter the bounds of the channels, whether that be 0-255 or 0-1000
 * 
 * @param r The red channel of the rgb value
 * @param g The green channel of the rgb value
 * @param b The blue channel of the rgb value
 * @return An integer representing the color channel value of all channels in the RGB grayscale representation of the inputted color
 */
int get_grayscale(int r, int g, int b);

/**
 * @brief Find the index of the closest color within a group of colors to the given input 
 * 
 * @param input The color to check against the color list
 * @param colors The color list to check for the closest color
 * @return The index of the closest color to **input** within the given color list
 * @throws std::runtime_error If the color list given is empty
 */
int find_closest_color_index(RGBColor& input, std::vector<RGBColor>& colors);

/**
 * @brief Find the closest color within a group of colors to the given input 
 * 
 * @param input The color to check against the color list
 * @param colors The color list to check for the closest color
 * @return The closest color to **input** within the given color list
 * @throws std::runtime_error If the color list given is empty
 */
RGBColor find_closest_color(RGBColor& input, std::vector<RGBColor>& colors);

/**
 * @brief Find the average color within a list
 * @param colors The colors to compile into one average color
 * @return The average color of all of the given colors.
 * @throws std::runtime_error if the given color list is empty
 */
RGBColor get_average_color(std::vector<RGBColor>& colors);

#endif
