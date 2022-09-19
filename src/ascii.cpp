#include <pixeldata.h>
#include <image.h>
#include <ascii.h>
#include <macros.h>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <ncurses.h>

extern "C" {
#include <libavutil/avutil.h>
} 

const int nb_val_chars = 11;
const char val_chars[nb_val_chars + 1] = "@\%#*+=-:._ ";


AsciiImage get_ascii_image_from_frame(AVFrame* videoFrame, int maxWidth, int maxHeight) {
    PixelData* data = pixel_data_alloc_from_frame(videoFrame);
    AsciiImage image = get_ascii_image_bounded(data, maxWidth, maxHeight);
    pixel_data_free(data);
    return image;
}

AsciiImage get_ascii_image_bounded(PixelData* pixelData, int maxWidth, int maxHeight) {
    int outputWidth, outputHeight;
    get_output_size(pixelData->width, pixelData->height, maxWidth, maxHeight, &outputWidth, &outputHeight);
    return get_ascii_image(pixelData->pixels, pixelData->width, pixelData->height, outputWidth, outputHeight, pixelData->format);
}

AsciiImage copy_ascii_image(AsciiImage* src) {
    AsciiImage dst;
    dst.width = src->width;
    dst.height = src->height;
    dst.colored = src->colored;
    
    for (int row = 0; row < MAX_ASCII_IMAGE_HEIGHT + 1; row++) {
        for (int col = 0; col < MAX_ASCII_IMAGE_WIDTH + 1; col++) {
            dst.lines[row][col] = src->lines[row][col];
            rgb_copy(dst.color_data[row][col], src->color_data[row][col]);
        }
    }

    return dst;
}

AsciiImage get_ascii_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight, PixelDataFormat pixel_format) {
  AsciiImage textImage;
  for (int i = 0; i < MAX_ASCII_IMAGE_HEIGHT + 1; i++) {
      for (int j = 0; j < MAX_ASCII_IMAGE_WIDTH + 1; j++) {
          textImage.lines[i][j] = ' ';
          rgb_set(textImage.color_data[i][j], 0, 0, 0);
      }
  }

  textImage.width = outputWidth;
  textImage.height = outputHeight;
  textImage.colored = pixel_format == RGB24 ? true : false;
  
  if (srcWidth <= outputWidth && srcHeight <= outputHeight) {
      for (int row = 0; row < outputHeight; row++) {
          for (int col = 0; col < outputWidth; col++) {

              if (pixel_format == GRAYSCALE8) {
                int pixel = row * srcWidth + col;
                textImage.lines[row][col] = get_char_from_value(pixels[pixel]);
              } else if (pixel_format == RGB24) {
                  int start_pixel = row * srcWidth * 3 + col * 3;
                  rgb values;
                  rgb_set(values, pixels[start_pixel], pixels[start_pixel + 1], pixels[start_pixel + 2]);
                  textImage.lines[row][col] = get_char_from_rgb(values);
                  rgb_copy(textImage.color_data[row][col], values);
              }

          }
          textImage.lines[row][outputWidth] = '\0';
      }
  } else {
    double scanWidth = (double)srcWidth / outputWidth;
    double scanHeight = (double)srcHeight / outputHeight;
    double currentRowPixel = 0;
    double currentColPixel = 0;

    for (int row = 0; row < outputHeight; row++) {
      currentColPixel = 0;
      int checkWidth, checkHeight;
      checkHeight = currentRowPixel != 0 ? (int)round(currentRowPixel - scanHeight * (row - 1)) : (int)scanHeight;

      for (int col = 0; col < outputWidth; col++) {
        checkWidth = currentColPixel != 0 ? (int)round(currentColPixel - scanWidth * (col - 1)) : (int)scanWidth;

        if (pixel_format == GRAYSCALE8) {
            textImage.lines[row][col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
        } else if (pixel_format == RGB24) {
            textImage.lines[row][col] = get_char_from_area_rgb(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
            get_avg_color_from_area_rgb(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight, textImage.color_data[row][col]);
        }

        currentColPixel += scanWidth;
      }

      textImage.lines[row][outputWidth] = '\0';
      currentRowPixel += scanHeight;
    }
  }

  return textImage;
}


char get_char_from_value(uint8_t value) {
  return val_chars[ (int)value * (nb_val_chars - 1) / 255 ];
}

char get_char_from_rgb(rgb colors) {
    return val_chars[get_grayscale_rgb(colors) * (nb_val_chars - 1) / 255];
}

char get_char_from_area(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight) {
  int value = 0;
  int valueCount = 0;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      int pixelIndex = (row + y) * pixelWidth + (x + col);
      if (pixelIndex < pixelWidth * pixelHeight && pixelIndex >= 0 &&  (x + col) < pixelWidth && (row + y) < pixelHeight  ) {
        value += pixels[pixelIndex];
        valueCount++;
      }
    }
  }

    return valueCount > 0 ? val_chars[ value * (nb_val_chars - 1) / (255 * valueCount) ] : val_chars[0];
}

char get_char_from_area_rgb(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight ) {
  int value = 0;
  int valueCount = 0;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      int pixelIndex = (row + y) * 3 * pixelWidth + (x + col) * 3;
      if (pixelIndex + 2 < pixelWidth * pixelHeight * 9 && pixelIndex >= 0 && (x + col) < pixelWidth && (row + y) < pixelHeight  ) {
        value += get_grayscale(pixels[pixelIndex], pixels[pixelIndex + 1], pixels[pixelIndex + 2]);
        valueCount++;
      }
    }
  }

    return valueCount > 0 ? val_chars[ value * nb_val_chars / (255 * valueCount) ] : val_chars[0];
}

void get_avg_color_from_area_rgb(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight, rgb output) {
  rgb colors[width * height];
  int nb_colors = 0;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      int pixelIndex = (row + y) * 3 * pixelWidth + (x + col) * 3;
      if (pixelIndex + 2 < pixelWidth * pixelHeight * 3 && pixelIndex >= 0 && (x + col) < pixelWidth && (row + y) < pixelHeight  ) {
          rgb_set(colors[nb_colors],  pixels[pixelIndex], pixels[pixelIndex + 1], pixels[pixelIndex + 2] );
          nb_colors++;
      }
    }
  }

  if (nb_colors > 0) {
      get_average_color(output, colors, nb_colors);
  } else {
      rgb_set(output, 0, 0, 0);
  }
}

void get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight, int* width, int* height) { 
    bool shouldShrink = srcWidth > targetWidth || srcHeight > targetHeight;
    double scaleFactor = shouldShrink ? std::min((double)targetWidth / srcWidth, (double)targetHeight / srcHeight) : std::max((double)targetWidth / srcWidth, (double)targetHeight / srcHeight);
    *width = (int)(srcWidth * scaleFactor);
    *height = (int)(srcHeight * scaleFactor);
}

void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height) {
  if (srcWidth <= maxWidth && srcHeight <= maxHeight) {
    *width = srcWidth;
    *height = srcHeight;
  } else {
      get_scale_size(srcWidth, srcHeight, maxWidth, maxHeight, width, height);
  }
}

void overlap_ascii_images(AsciiImage* first, AsciiImage* second) {
  int topLeftX = (first->width - second->width) / 2;
  int topLeftY = (first->height - second->height) / 2;

  for (int row = 0; row < second->height; row++) {
    for (int col = 0; col < second->width; col++) {
      if (topLeftY + row < first->height && topLeftX + col < first->width && second->lines[row][col] != '\0') {
        first->lines[topLeftY + row][topLeftX + col] = second->lines[row][col];
        rgb_copy(first->color_data[topLeftY + row][topLeftX + col], second->color_data[row][col]);
      }
    }
  }
}

