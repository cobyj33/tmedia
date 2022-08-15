#include <ascii.h>
#include <cstdint>
#include <cmath>
#include <stdexcept>



#if defined(__linux__)
  #include <sys/ioctl.h>
  #include <unistd.h>
#elif defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
#endif

#include <ncurses.h>

ascii_image get_image(uint8_t* pixels, int srcWidth, int srcHeight, int outputWidth, int outputHeight) {
  // int windowWidth, windowHeight;
  // get_window_size(&windowWidth, &windowHeight);
  ascii_image textImage;
  // printw("Stats: SrcWidth: %d SrcHeight: %d outputWidth: %d outputHeight: %d \n", srcWidth, srcHeight, outputWidth, outputHeight);
  textImage.width = outputWidth;
  textImage.height = outputHeight;
  
  if (srcWidth <= outputWidth && srcHeight <= outputHeight) {
    // addstr("First Algorithm\n");
      for (int row = 0; row < outputHeight; row++) {
          for (int col = 0; col < outputWidth; col++) {
            int pixel = row * srcWidth + col;
            textImage.lines[row][col] = get_char_from_value(pixels[pixel]);
          }
          textImage.lines[row][outputWidth] = '\0';
      }
      
  } else {
    // addstr("Second Algorithm\n");
    double scanWidth = (double)srcWidth / outputWidth;
    double scanHeight = (double)srcHeight / outputHeight;
    double currentRowPixel = 0;
    double currentColPixel = 0;

    int lastCheckedRow = 0;
    int lastCheckedCol = 0;

    for (int row = 0; row < outputHeight; row++) {
      // char line[outputWidth + 1];
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
      // line[outputWidth] = '\0';
      // textImage.lines.push_back(line);
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
    printw("%s\n", lineAcross.c_str());
  }

  for (int row = 0; row < textImage->height; row++) {
    printw("%s%s\n", horizontalPadding.c_str(), textImage->lines[row]);
  }
}

char get_char_from_value(uint8_t value) {
  return value_characters[ value * (value_characters.length() - 1) / 255 ];
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
    return value_characters[ value * (value_characters.length() - 1) / (255 * valueCount) ];
  } else {
    return value_characters[0];
  }

}


void get_window_size(int* width, int* height) {
  #if defined(_WIN32) || defined(_WIN64)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  #elif defined(__linux__)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *width = w.ws_col;
    *height = w.ws_row;
  #endif
}

void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height) {
  if (srcWidth <= maxWidth && srcHeight <= maxHeight) {
    *width = srcWidth;
    *height = srcHeight;
  } else {
    double shrinkFactor = std::min((double)maxWidth / srcWidth, (double)maxHeight / srcHeight);
    *width = (int)(srcWidth * shrinkFactor);
    *height = (int)(srcHeight * shrinkFactor);
  }
}

void overlap_ascii_images(ascii_image* first, ascii_image* second) {

  int topLeftX = (first->width - second->width) / 2;
  int topLeftY = (first->height - second->height) / 2;

  for (int row = 0; row < second->height; row++) {
    for (int col = 0; col < second->width; col++) {
      if (topLeftY + row < first->height && topLeftX + col < first->width) {
        first->lines[topLeftY + row][topLeftX + col] = second->lines[row][col];
      }
    }
  }
  

}