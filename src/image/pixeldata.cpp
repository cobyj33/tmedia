#include <tmedia/image/pixeldata.h>

#include <tmedia/image/color.h>
#include <tmedia/util/wmath.h>
#include <tmedia/image/scale.h>
#include <tmedia/ffmpeg/decode.h>
#include <tmedia/ffmpeg/boiler.h>
#include <tmedia/ffmpeg/videoconverter.h>
#include <tmedia/ffmpeg/ffmpeg_error.h>

#include <cstdint>
#include <cstring>
#include <cassert>

#include <fmt/format.h>

extern "C" {
  #include <libavutil/frame.h>
}

void pixdata_setnewdims(PixelData& dest, int width, int height) {
  assert(width >= 0);
  assert(height >= 0);

  dest.m_width = width;
  dest.m_height = height;
  dest.pixels.resize(width * height);
}

void pixdata_initgray(PixelData& dest, int width, int height, std::uint8_t g) {
  assert(width >= 0);
  assert(height >= 0);

  pixdata_setnewdims(dest, width, height);
  std::memset(dest.pixels.data(), g, dest.pixels.size() * sizeof(RGB24));
}

void pixdata_copy(PixelData& dest, PixelData& src) {
  pixdata_setnewdims(dest, src.m_width, src.m_height);
  std::memcpy(dest.pixels.data(), src.pixels.data(), dest.pixels.size() * sizeof(RGB24));
}

void vertline(PixelData& dest, int row1, int row2, int col, const RGB24 color) {
  const int rowmin = std::min(row1, row2);
  const int rowmax = std::max(row1, row2);

  for (int row = rowmin; row <= rowmax; row++) {
    dest.pixels[row * dest.m_width + col] = color;
  }
}

void horzline(PixelData& dest, int col1, int col2, int row, const RGB24 color) {
  const int colmax = std::max(col1, col2);
  const int colmin = std::min(col1, col2);

  for (int col = colmin; row <= colmax; row++) {
    dest.pixels[row * dest.m_width + col] = color;
  }
}

void pixdata_from_avframe(PixelData& dest, AVFrame* src) {
  assert(static_cast<AVPixelFormat>(src->format) == AV_PIX_FMT_RGB24);
  pixdata_setnewdims(dest, src->width, src->height);

  // if our RGB24 compiles to 3 bytes (like it should)
  // we can just memcpy in directly
  if constexpr (sizeof(RGB24) == 3) {
    // apparently you can't just assign to ranges outside of
    // [vec.begin(), vec.begin() + vec.size()]
    // https://stackoverflow.com/a/7689457
    // also adjusts the pixel array's size() to be correct

    const uint8_t* const data = src->data[0];
    std::memcpy(dest.pixels.data(), data, dest.m_width * dest.m_height * 3);
  } else {
    // this shouldn't really run on most systems. It's moreso here as
    // a backup safety
    // Since RGB24 data is not tightly packed, we have to copy each
    // rgb triplet individually

    std::size_t di = 0;
    const uint8_t* const data = src->data[0];
    const int area = dest.m_width * dest.m_height;
    for (int i = 0; i < area; i++) {
      dest.pixels[i] = { data[di], data[di + 1], data[di + 2] };
      di += 3;
    }
  }
}
