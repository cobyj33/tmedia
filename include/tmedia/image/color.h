#ifndef TMEDIA_COLOR_H
#define TMEDIA_COLOR_H

#include <cstdint>

/**
 * @brief Get the grayscale value of 3 channels of rgb data.
 * 
 * @note This function should work for r >= 0, g >= 0, b >= 0, no matter
 * the bounds of the channels, whether that be 0-255 or 0-1000
 * 
 * @param r The r channel of the rgb value
 * @param g The g channel of the rgb value
 * @param b The b channel of the rgb value
 * @return An integer representing the color channel value of all channels in
 * the RGB grayscale representation of the inputted color
 */
[[gnu::always_inline]] constexpr inline std::uint8_t get_gray8(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  return static_cast<std::uint8_t>(0.299 * static_cast<double>(r) + 0.587 * static_cast<double>(g) + 0.114 * static_cast<double>(b));
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

    RGB24() = default;
    constexpr RGB24(std::uint8_t gray) : r(gray), g(gray), b(gray) {}
    constexpr RGB24(std::uint8_t r, std::uint8_t g, std::uint8_t b) : r(r), g(g), b(b) {}

    [[gnu::always_inline]] constexpr inline bool equals(RGB24 other) const {
      return this->r == other.r && this->g == other.g && this->b == other.b;
    }

    [[gnu::always_inline]] constexpr inline bool operator==(RGB24 other) const {
      return this->r == other.r && this->g == other.g && this->b == other.b;
    }

    double distance(RGB24 other) const;
    double dis_sq(RGB24 other) const;

    [[gnu::always_inline]] constexpr inline RGB24 get_comp() const {
      return RGB24(255 - this->r, 255 - this->g, 255 - this->b );
    }

    [[gnu::always_inline]] constexpr inline bool is_gray() const {
      return this->r == this->g && this->g == this->b;
    }


    [[gnu::always_inline]] constexpr inline RGB24 as_gray() const {
      return RGB24(get_gray8(this->r, this->g, this->b));
    }

    [[gnu::always_inline]] constexpr inline std::uint8_t gray_val() const {
      return get_gray8(this->r, this->g, this->b);
    }
};

constexpr RGB24 operator "" _rgb(unsigned long long val) {
  return RGB24(static_cast<std::uint8_t>((val & 0x00FF0000) >> 16),
    static_cast<std::uint8_t>((val & 0x0000FF00) >> 8),
    static_cast<std::uint8_t>(val & 0x000000FF));
}

#endif
