#include <image.h>
#include <ascii.h>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <ncurses.h>

extern "C" {
#include <libavutil/avutil.h>
} 

const int nb_val_chars = 12;
const char val_chars[nb_val_chars] = "@%#*+=-:._ ";

ascii_image get_ascii_image_from_frame(AVFrame* videoFrame, int maxWidth, int maxHeight) {
    pixel_data* data = pixel_data_alloc_from_frame(videoFrame);
    ascii_image image = get_ascii_image_bounded(data, maxWidth, maxHeight);
    pixel_data_free(data);
    return image;
}

ascii_image get_ascii_image_bounded(pixel_data* pixelData, int maxWidth, int maxHeight) {
    int outputWidth, outputHeight;
    get_output_size(pixelData->width, pixelData->height, maxWidth, maxHeight, &outputWidth, &outputHeight);
    return get_ascii_image(pixelData->pixels, pixelData->width, pixelData->height, outputWidth, outputHeight);
}

ascii_image copy_ascii_image(ascii_image* src) {
    ascii_image dst;
    dst.width = src->width;
    dst.height = src->height;
    
    for (int row = 0; row < src->height; row++) {
        for (int col = 0; col < src->width; col++) {
            dst.lines[row][col] = src->lines[row][col];
        }
    }

    return dst;
}

ascii_image get_ascii_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight) {
  ascii_image textImage;
  for (int i = 0; i < MAX_FRAME_HEIGHT + 1; i++) {
      for (int j = 0; j < MAX_FRAME_WIDTH + 1; j++) {
          textImage.lines[i][j] = ' ';
      }
  }

  textImage.width = outputWidth;
  textImage.height = outputHeight;
  
  if (srcWidth <= outputWidth && srcHeight <= outputHeight) {
      for (int row = 0; row < outputHeight; row++) {
          for (int col = 0; col < outputWidth; col++) {
            int pixel = row * srcWidth + col;
            textImage.lines[row][col] = get_char_from_value(pixels[pixel]);
          }
          textImage.lines[row][outputWidth] = '\0';
      }
  } else {
    double scanWidth = (double)srcWidth / outputWidth;
    double scanHeight = (double)srcHeight / outputHeight;
    double currentRowPixel = 0;
    double currentColPixel = 0;

    int lastCheckedRow = 0;
    int lastCheckedCol = 0;

    for (int row = 0; row < outputHeight; row++) {
      currentColPixel = 0;
      int checkWidth, checkHeight;
      checkHeight = currentRowPixel != 0 ? (int)round(currentRowPixel - scanHeight * (row - 1)) : (int)scanHeight;
      for (int col = 0; col < outputWidth; col++) {

        checkWidth = currentColPixel != 0 ? (int)round(currentColPixel - scanWidth * (col - 1)) : (int)scanWidth;

        textImage.lines[row][col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
        lastCheckedCol = (int)(currentColPixel + checkWidth);
        currentColPixel += scanWidth;
      }
      textImage.lines[row][outputWidth] = '\0';
      lastCheckedRow = (int)(currentRowPixel + checkHeight);
      currentRowPixel += scanHeight;
    }
  }


  return textImage;
}


char get_char_from_value(uint8_t value) {
  return val_chars[ value * (nb_val_chars - 1) / 255 ];
}

char get_char_from_area(uint8_t* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight ) {
  if (width == 0 && height == 0) {
    throw std::invalid_argument("CANNOT GET CHAR FROM AREA WITH NO WIDTH");
  } else if (width == 0) {
    throw std::invalid_argument("CANNOT GET CHAR FROM AREA WITH NO WIDTH");
  } else if (height == 0) {
    throw std::invalid_argument("CANNOT GET CHAR FROM AREA WITH NO HEIGHT");
  }

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

  if (valueCount > 0) {
    return val_chars[ value * (nb_val_chars - 1) / (255 * valueCount) ];
  } else {
    return val_chars[0];
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
    /* double shrinkFactor = std::min((double)maxWidth / srcWidth, (double)maxHeight / srcHeight); */
    /* *width = (int)(srcWidth * shrinkFactor); */
    /* *height = (int)(srcHeight * shrinkFactor); */
  }
}

void overlap_ascii_images(ascii_image* first, ascii_image* second) {
  int topLeftX = (first->width - second->width) / 2;
  int topLeftY = (first->height - second->height) / 2;

  for (int row = 0; row < second->height; row++) {
    for (int col = 0; col < second->width; col++) {
      if (topLeftY + row < first->height && topLeftX + col < first->width && second->lines[row][col] != '\0') {
        first->lines[topLeftY + row][topLeftX + col] = second->lines[row][col];
      }
    }
  }

}

