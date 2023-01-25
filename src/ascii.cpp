
#include <image.h>
#include <pixeldata.h>
#include <ascii.h>
#include <cstdint>
#include <wmath.h>

extern "C" {
#include <ncurses.h>
#include <libavutil/avutil.h>
}

const int nb_val_chars = 12;
const char val_chars[12] = "@%#*+=-:._ ";

AsciiImage* get_ascii_image_from_frame(AVFrame* videoFrame, int maxWidth, int maxHeight) {
    PixelData* data = pixel_data_alloc_from_frame(videoFrame);
    AsciiImage* image = get_ascii_image_bounded(data, maxWidth, maxHeight);
    pixel_data_free(data);
    return image;
}

AsciiImage* get_ascii_image_bounded(PixelData* pixelData, int maxWidth, int maxHeight) {
    int outputWidth, outputHeight;
    get_output_size(pixelData->width, pixelData->height, maxWidth, maxHeight, &outputWidth, &outputHeight);
    return get_ascii_image(pixelData->pixels, pixelData->width, pixelData->height, outputWidth, outputHeight, pixelData->format);
}


AsciiImage* ascii_image_alloc(int width, int height, int colored) {
    AsciiImage* dst = (AsciiImage*)malloc(sizeof(AsciiImage));
    dst->width = width;
    dst->height = height;
    dst->colored = colored;
    dst->lines = colored ? (char*)malloc(sizeof(char) * width * height * 3) : (char*)malloc(sizeof(char) * width * height);
    if (dst->lines == NULL) {
        free(dst);
        return NULL;
    }

    if (colored) {
        dst->color_data = (rgb*)malloc(sizeof(rgb) * width * height);
        if (dst->color_data == NULL) {
            free(dst->lines);
            free(dst);
            return NULL;
        }
    } else {
        dst->color_data = NULL;
    }

  for (int i = 0; i < height; i++) {
      for (int j = 0; j < width; j++) {
          dst->lines[i * width + j] = ' ';
          if (colored) {
              rgb_set(dst->color_data[i * width + j], 0, 0, 0);
          }
      }
  }

    return dst;
}

void ascii_image_free(AsciiImage* image) {
    free(image->lines);
    if (image->color_data != NULL) {
        free(image->color_data);
    }
    free(image);
}

AsciiImage* copy_ascii_image(AsciiImage* src) {
    AsciiImage* dst = ascii_image_alloc(src->width, src->height, src->colored);
    if (dst == NULL) {
        return NULL;
    }
    
    for (int row = 0; row < src->width; row++) {
        for (int col = 0; col < src->height; col++) {
            dst->lines[row * dst->width + col] = src->lines[row * src->width + col];
            rgb_copy(dst->color_data[row * dst->width + col], src->color_data[row * src->width + col]);
        }
    }

    return dst;
}

AsciiImage* get_ascii_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight, PixelDataFormat pixel_format) {
  AsciiImage* textImage = ascii_image_alloc(outputWidth, outputHeight, pixel_format == RGB24 ? true : false);
  if (textImage == NULL) {
      return NULL;
  }

  if (srcWidth <= outputWidth && srcHeight <= outputHeight) {
      for (int row = 0; row < outputHeight; row++) {
          for (int col = 0; col < outputWidth; col++) {

              if (pixel_format == GRAYSCALE8) {
                int pixel = row * srcWidth + col;
                textImage->lines[row * outputWidth + col] = get_char_from_value(pixels[pixel]);
              } else if (pixel_format == RGB24) {
                  int start_pixel = row * srcWidth * 3 + col * 3;
                  rgb values;
                  rgb_set(values, pixels[start_pixel], pixels[start_pixel + 1], pixels[start_pixel + 2]);
                  textImage->lines[row * outputWidth + col] = get_char_from_rgb(values);
                  rgb_copy(textImage->color_data[row * outputWidth + col], values);
              }

          }
          /* textImage->lines[rooutputWidth] = '\0'; */
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
            textImage->lines[row * outputWidth + col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
        } else if (pixel_format == RGB24) {
            textImage->lines[row * outputWidth + col] = get_char_from_area_rgb(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
            get_avg_color_from_area_rgb(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight, textImage->color_data[row * outputWidth + col]);
        }

        currentColPixel += scanWidth;
      }

      /* textImage.lines[row][outputWidth] = '\0'; */
      currentRowPixel += scanHeight;
    }
  }

  return textImage;
}


char get_char_from_value(uint8_t value) {
  return val_chars[ (int)value * (nb_val_chars - 1) / 255 ];
}

int get_rgb_from_char(rgb output, char ch) {
    for (int i = 0; i < nb_val_chars; i++) {
        if (ch == val_chars[i]) {
            uint8_t val = i * 255 / (nb_val_chars - 1); 
            rgb_set(output, val, val, val);
            return 1;
        }
    }
    return 0;
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
    if (width * height <= 0) {
        rgb black = { 0, 0, 0 };
        rgb_copy(output, black);
        return;
    }

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
    double scaleFactor = shouldShrink ? fmin((double)targetWidth / srcWidth, (double)targetHeight / srcHeight) : fmax((double)targetWidth / srcWidth, (double)targetHeight / srcHeight);
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
      if (topLeftY + row < first->height && topLeftX + col < first->width && second->lines[row * second->width + col] != '\0') {
        first->lines[(topLeftY + row) * first->width + topLeftX + col] = second->lines[row * second->width + col];
        if (first->colored && second->colored) {
            rgb_copy(first->color_data[(topLeftY + row) * first->width + topLeftX + col], second->color_data[row * second->width + col]);
        }

      }
    }
  }
}

int ascii_init_color(AsciiImage* image) {
    if (image->colored) {
        return 1;
    }

    if (image->color_data != NULL) {
        free(image->color_data);
    }

    image->color_data = (rgb*)malloc(sizeof(rgb) * image->width * image->height);
    if (image->color_data == NULL) {
        return 0;
    }

    rgb output;
    for (int i = 0; i < image->width * image->height; i++) {
        get_rgb_from_char(output, image->lines[i]);
        rgb_copy(image->color_data[i], output);
    }
    image->colored = true;
    return 1;
}

int ascii_fill_color(AsciiImage* image, rgb color) {
    if (!image->colored) {
        int result = ascii_init_color(image);
        if (!result) {
            fprintf(stderr, "%s", "Could not convert grayscale ascii image to color");
            return 0;
        }
    }

    for (int i = 0; i < image->width * image->height; i++) {
        rgb_copy(image->color_data[i], color);
    }
    return 1;
}

