#include <ascii.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>


ascii_image get_image(Color* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight) {
  int windowWidth, windowHeight;
  get_window_size(&windowWidth, &windowHeight);
  ascii_image textImage;
  textImage.width = outputWidth;
  textImage.height = outputHeight;
  
  if (srcWidth <= outputWidth && srcHeight <= outputHeight) {

      for (int row = 0; row < outputHeight; row++) {
        char line[outputWidth + 1];
          for (int col = 0; col < outputWidth; col++) {
            line[col] = get_char_from_values(pixels[row * srcWidth + col]);
          }
          line[outputWidth] = '\0';
          textImage.lines.push_back(line);
      }
      
  } else {
    double scanWidth = (double)srcWidth / outputWidth;
    double scanHeight = (double)srcHeight / outputHeight;
    double currentRowPixel = 0;
    double currentColPixel = 0;

    int lastCheckedRow = 0;
    int lastCheckedCol = 0;

    for (int row = 0; row < outputHeight; row++) {
      char line[outputWidth + 1];
      currentColPixel = 0;
      int checkWidth, checkHeight;
      checkHeight = currentRowPixel != 0 ? (int)round(currentRowPixel - scanHeight * (row - 1)) : (int)scanHeight;
      for (int col = 0; col < outputWidth; col++) {

        checkWidth = currentColPixel != 0 ? (int)round(currentColPixel - scanWidth * (col - 1)) : (int)scanWidth;

        line[col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, srcWidth, srcHeight);
        lastCheckedCol = (int)(currentColPixel + checkWidth);
        currentColPixel += scanWidth;
      }

      line[outputWidth] = '\0';
      textImage.lines.push_back(line);
      lastCheckedRow = (int)(currentRowPixel + checkHeight);
      currentRowPixel += scanHeight;
    }
  }


  return textImage;
}

void print_ascii_image(ascii_image* textImage) {
  int windowWidth, windowHeight;
  move(0, 0);
  get_window_size(&windowWidth, &windowHeight);

  std::string horizontalPadding((windowWidth - textImage->width) / 2, ' ');
  std::string lineAcross(windowWidth, ' ');
  for (int i = 0; i < (windowHeight - textImage->height) / 2; i++) {
    addstr(lineAcross.c_str());
    addch('\n');
  }

  for (int row = 0; row < textImage->height; row++) {
    addstr(horizontalPadding.c_str());
    addstr(textImage->lines[row].c_str());
    addch('\n');
  }
}

void get_pixels(cv::Mat* image, Color* pixels, int width, int height) {
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      cv::Vec3b pixel = image->at<cv::Vec3b>(row, col);
      pixels[row * width + col] = { pixel[2], pixel[1], pixel[0] };
    }
  }
}

char get_char_from_values(Color values) {
  int value = get_grayscale(values.r, values.g, values.b);

  return value_characters[ value * (value_characters.length() - 1) / 255 ];
}

char get_char_from_area(Color* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight ) {
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
      if (pixelIndex < pixelWidth * pixelHeight && pixelIndex >= 0) {
        value += get_grayscale(pixels[pixelIndex].r, pixels[pixelIndex].g, pixels[pixelIndex].b);
        valueCount++;
      }
    }
  }

  if (valueCount > 0) {
    return value_characters[ value * (value_characters.length() - 1) / (255 * valueCount) ];
  } else {
    return value_characters[0];
  }

}

void get_window_size(int* width, int* height) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  *width = w.ws_col;
  *height = w.ws_row;
}

int get_grayscale(int r, int g, int b) {
  return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

void get_output_size(int srcWidth, int srcHeight, int* width, int* height) {
  int windowWidth, windowHeight;
  get_window_size(&windowWidth, &windowHeight);

  if (srcWidth <= windowWidth && srcHeight <= windowHeight) {
    *width = srcWidth;
    *height = srcHeight;
  } else {
    double shrinkFactor = std::min((double)windowWidth / srcWidth, (double)windowHeight / srcHeight);
    *width = (int)(srcWidth * shrinkFactor);
    *height = (int)(srcHeight * shrinkFactor);
  }
}
