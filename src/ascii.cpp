#include <ascii.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>

/* ascii_image get_image_from_pixels(cv::Vec3b* pixels, int imageWidth, int imageHeight, int outputWidth, int outputHeight) { */
    
/* } */


ascii_image get_image(cv::Mat* target, int outputWidth, int outputHeight) {
  int windowWidth, windowHeight;
  cv::Mat image = *target;
  cv::Vec3b pixels[image.rows * image.cols];
  get_pixels(target, pixels, image.cols, image.rows);
  get_window_size(&windowWidth, &windowHeight);



  ascii_image textImage;
  textImage.width = outputWidth;
  textImage.height = outputHeight;

  if (image.cols <= outputWidth && image.rows <= outputHeight) {

      for (int row = 0; row < outputHeight; row++) {
        char line[outputWidth + 1];
          for (int col = 0; col < outputWidth; col++) {
            line[col] = get_char_from_values(pixels[row * image.cols + col]);
          }
          line[outputWidth] = '\0';
          textImage.lines.push_back(line);
      }
      
  } else {
    double scanWidth = (double)image.cols / outputWidth;
    double scanHeight = (double)image.rows / outputHeight;

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

        line[col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, image.cols, image.rows);
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

void print_pixels(int *pixels, int width, int height) {

}

void print_ascii_image(ascii_image textImage) {
  int windowWidth, windowHeight;
  move(0, 0);
  get_window_size(&windowWidth, &windowHeight);

  std::string horizontalPadding((windowWidth - textImage.width) / 2, ' ');
  std::string lineAcross(windowWidth, ' ');
  for (int i = 0; i < (windowHeight - textImage.height) / 2; i++) {
    addstr(lineAcross.c_str());
    addch('\n');
  }

  for (int row = 0; row < textImage.height; row++) {
    addstr(horizontalPadding.c_str());
    addstr(textImage.lines[row].c_str());
    addch('\n');
  }
}


void print_image(cv::Mat* target) {
  int windowWidth, windowHeight;
  move(0, 0);
  get_window_size(&windowWidth, &windowHeight);

  int outputWidth, outputHeight;
  get_output_size(target, &outputWidth, &outputHeight);

  std::string horizontalPadding((windowWidth - outputWidth) / 2, ' ');
  std::string lineAcross(windowWidth, ' ');
  for (int i = 0; i < (windowHeight - outputHeight) / 2; i++) {
    addstr(lineAcross.c_str());
    addch('\n');
  }

  ascii_image textImage = get_image(target, outputWidth, outputHeight);
  for (int row = 0; row < textImage.height; row++) {
    addstr(horizontalPadding.c_str());
    addstr(textImage.lines[row].c_str());
    addch('\n');
  }
}

void get_pixels(cv::Mat* image, cv::Vec3b* pixels, int width, int height) {
  cv::Mat readImage = *image;

  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      pixels[row * width + col] = readImage.at<cv::Vec3b>(row, col);
    }
  }
}

char get_char_from_values(cv::Vec3b values) {
  int value = get_grayscale(values[0], values[1], values[2]);

  return value_characters[ value * (value_characters.length() - 1) / 255 ];
}

char get_char_from_area(cv::Vec3b* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight ) {
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
      if (pixelIndex < pixelWidth * pixelHeight && pixelIndex > 0) {
        cv::Vec3b currentPixel = pixels[pixelIndex];
        value += get_grayscale(currentPixel[0], currentPixel[1], currentPixel[2]);
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

void get_output_size(cv::Mat* target, int* width, int* height) {
  int windowWidth, windowHeight;
  cv::Mat image = *target;
  get_window_size(&windowWidth, &windowHeight);

  if (image.cols <= windowWidth && image.rows <= windowHeight) {
    *width = image.cols;
    *height = image.rows;
  } else {
    double shrinkFactor = std::min((double)windowWidth / image.cols, (double)windowHeight / image.rows);
    *width = (int)(image.cols * shrinkFactor);
    *height = (int)(image.rows * shrinkFactor);
  }

}
