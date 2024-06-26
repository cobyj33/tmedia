#ifndef TMEDIA_COLOR_H
#define TMEDIA_COLOR_H

#include <vector>
#include <cstddef>
#include <cstdint>
#include <tmedia/util/defines.h>

TMEDIA_ALWAYS_INLINE inline std::uint8_t get_gray8(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return static_cast<std::uint8_t>(0.299 * static_cast<double>(r) + 0.587 * static_cast<double>(g) + 0.114 * static_cast<double>(b));
}



TMEDIA_ALWAYS_INLINE inline int get_grayint(int r, int g, int b) {
  return static_cast<int>(0.299 * static_cast<double>(r) + 0.587 * static_cast<double>(g) + 0.114 * static_cast<double>(b));
}


/**
 * @brief A representation of an RGB Color with R, G, and B channels in the
 * range 0-255
 * 
 * Note that while RGB24 assumes that colors are in the range 0-255, colors
 * in different ranges can still be stored inside of a RGB24 object. The user
 * should strongly stick to using color channels in the range 0-255 inclusive though
 * and only convert to different ranges when completely necessary, such as interacting
 * with curses colors in the range 0-1000.
 */
class RGB24 {
  public:
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    static RGB24 BLACK;
    static RGB24 WHITE; 

    RGB24() : r(0), g(0), b(0) {}
    RGB24(std::uint8_t gray) : r(gray), g(gray), b(gray) {}
    RGB24(std::uint8_t r, std::uint8_t g, std::uint8_t b) : r(r), g(g), b(b) {}
    RGB24(const RGB24& color) : r(color.r), g(color.g), b(color.b) {}
    RGB24(RGB24&& color) : r(color.r), g(color.g), b(color.b) {}

    TMEDIA_ALWAYS_INLINE inline void operator=(const RGB24& color) {
      this->r = color.r;
      this->g = color.g;
      this->b = color.b;
    }

    TMEDIA_ALWAYS_INLINE inline void operator=(RGB24&& color) {
      this->r = color.r;
      this->g = color.g;
      this->b = color.b;
    }

    TMEDIA_ALWAYS_INLINE inline bool equals(const RGB24& other) const {
      return this->r == other.r && this->g == other.g && this->b == other.b;
    }

    TMEDIA_ALWAYS_INLINE inline bool operator==(const RGB24& other) const {
      return this->r == other.r && this->g == other.g && this->b == other.b;
    }

    TMEDIA_ALWAYS_INLINE inline bool operator==(RGB24&& other) const {
      return this->r == other.r && this->g == other.g && this->b == other.b;
    }

    double distance(const RGB24& other) const;
    double dis_sq(const RGB24& other) const;

    TMEDIA_ALWAYS_INLINE inline RGB24 get_comp() const {
      return RGB24(255 - this->r, 255 - this->g, 255 - this->b );
    } 

    TMEDIA_ALWAYS_INLINE inline bool is_gray() const {
      return this->r == this->g && this->g == this->b;
    }


    TMEDIA_ALWAYS_INLINE inline RGB24 as_gray() const {
      return RGB24(get_gray8(this->r, this->g, this->b));
    }

    TMEDIA_ALWAYS_INLINE inline std::uint8_t gray_val() const {
      return get_gray8(this->r, this->g, this->b);
    }
};



class RGBColorHashFunction {
public:
    // Ripped from http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf pg 666 sec 4.1 paragraph 2
    std::size_t operator()(const RGB24& rgb) const {
      return (73856093UL * static_cast<std::size_t>(rgb.r)) ^
            (19349663UL * static_cast<std::size_t>(rgb.g)) ^
            (83492791UL * static_cast<std::size_t>(rgb.b));
    }
};

/**
 * @brief Get the grayscale value of 3 channels of rgb data.
 * 
 * @note This function should work for r >= 0, g >= 0, b >= 0, no matter the bounds of the channels, whether that be 0-255 or 0-1000
 * 
 * @param r The r channel of the rgb value
 * @param g The g channel of the rgb value
 * @param b The b channel of the rgb value
 * @return An integer representing the color channel value of all channels in the RGB grayscale representation of the inputted color
 */
int get_grayint(int r, int g, int b);
std::uint8_t get_gray8(std::uint8_t r, std::uint8_t g, std::uint8_t b);

/**
 * @brief Find the index of the closest color within a group of colors to the given input 
 * 
 * @param input The color to check against the color list
 * @param colors The color list to check for the closest color
 * @return The index of the closest color to **input** within the given color list
 * @throws std::runtime_error If the color list given is empty
 */
int find_closest_color_index(RGB24& input, std::vector<RGB24>& colors);

/**
 * @brief Find the closest color within a group of colors to the given input 
 * 
 * @param input The color to check against the color list
 * @param colors The color list to check for the closest color
 * @return The closest color to **input** within the given color list
 * @throws std::runtime_error If the color list given is empty
 */
RGB24 find_closest_color(RGB24& input, std::vector<RGB24>& colors);

/**
 * @brief Find the average color within a list
 * @param colors The colors to compile into one average color
 * @return The average color of all of the given colors.
 * @throws std::runtime_error if the given color list is empty
 */
RGB24 get_average_color(std::vector<RGB24>& colors);

#endif
