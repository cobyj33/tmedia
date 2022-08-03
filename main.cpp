#include <sys/ioctl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#include <curses.h>

#include <thread>
#include <chrono>
#include <iostream>
#include <string>

void print_image(cv::Mat* image);
void get_pixels(cv::Mat* image, cv::Vec3b* pixels, int width, int height);
char get_char_from_values(cv::Vec3b values);
char get_char_from_area(cv::Vec3b* pixels, int x, int y, int width, int height, int pixelWidth, int pixelHeight );
void getWindowSize(int* width, int* height);
void get_output_size(cv::Mat* target, int* width, int* height);

int imageProgram(int argc, char** argv);
int videoProgram(int argc, char** argv);
int average(int argc, int* args);
int get_grayscale(int r, int g, int b);

int windowWidth = 0;
int windowHeight = 0;

int main(int argc, char** argv)
{
  initscr();
  return videoProgram(argc, argv);
}

int videoProgram(int argc, char** argv) {
  cv::VideoCapture capture("/home/cobyj33/One Piece Dub Episode 1.mp4");

  if (!capture.isOpened()) {
    std::cout << "Could not open video";
    system("pause");
    return EXIT_FAILURE;
  }


  while (true) {
    cv::Mat frame;
    capture >> frame;

    if (frame.empty()) {
      break;
    }

    erase();
    print_image(&frame);
    refresh();
    // clear();
    // std::cout << "\\033[%d;%dH00";
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 24));
    // cv::waitKey(1000 / 24);
  }

  capture.release();
  return EXIT_SUCCESS;
}

int imageProgram(int argc, char** argv) {
 // Read the image file
  cv::Mat image = cv::imread("face.png");
  if (image.empty()) // Check for failure
  {
   std::cout << "Could not open or find the image" << std::endl;
   system("pause"); //wait for any key press
   return -1;
  }


  return 0;
}


void print_image(cv::Mat* target) {
  cv::Mat image = *target;
  cv::Vec3b pixels[image.rows * image.cols];
  move(0, 0);
  get_pixels(target, pixels, image.cols, image.rows);
  getWindowSize(&windowWidth, &windowHeight);

  int outputWidth;
  int outputHeight;
  get_output_size(target, &outputWidth, &outputHeight);

  //image is bigger than output terminal
  if (image.cols <= outputWidth && image.rows <= outputHeight) {

      for (int row = 0; row < outputHeight; row++) {
        char line[outputWidth];
          for (int col = 0; col < outputWidth; col++) {
            line[col] = get_char_from_values(pixels[row * image.cols + col]);
          }
          addstr(line);
          addch('\n');
          // std::cout << "width " << outputWidth << std::endl;
          // std::cout << "height " << outputHeight << std::endl;
      }
      
  } else {

    double scanWidth = image.cols / outputWidth;
    double scanHeight = image.rows / outputHeight;

    double currentRowPixel = 0;
    double currentColPixel = 0; 


    for (int row = 0; row < outputHeight; row++) {
      char line[outputWidth];
      currentColPixel = 0;
      for (int col = 0; col < outputWidth; col++) {

        int checkWidth = currentColPixel != 0 ? (int)(currentColPixel - scanWidth * (col - 1)) : (int)scanWidth;
        int checkHeight = currentRowPixel != 0 ? (int)(currentRowPixel - scanHeight * (row - 1)) : (int)scanHeight;

        line[col] = get_char_from_area(pixels, (int)currentColPixel, (int)currentRowPixel, checkWidth, checkHeight, image.cols, image.rows);

        currentColPixel += scanWidth;
        if (currentColPixel + scanWidth > image.cols) {
          break;
        }
      }
      addstr(line);
      addch('\n');
      currentRowPixel += scanHeight;
      if (currentRowPixel + scanHeight > image.rows) {
        break;
      }
    }

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

int average(int argc, int* argv) {

  if (argc <= 0) {
    throw std::invalid_argument("ERROR: ATTEMPT TO AVERAGE ARRAY OF 0 INTS");
  }

  int sum;
  for (int i = 0; i < argc; i++) {
    sum += argv[i];
  }

  return sum / argc;
}

// const int value_characters_length = 95;
// const char value_characters[value_characters_length] = "@MBHENR#KWXDFPQASUZbdehx*8Gm&04LOVYkpq5Tagns69owz$CIu23Jcfry\%1v7l+it[]{}?j|()=~!-/<>\"^_';,:`.";
// const int value_characters_length = 8;
// const char value_characters[value_characters_length] = "█▉▊▋▌▍▎▏";

const int value_characters_length = 11;
const char value_characters[value_characters_length] = "@\%#*+=-:. ";

char get_char_from_values(cv::Vec3b values) {
  int value = get_grayscale(values[0], values[1], values[2]);

  return value_characters[ value * (value_characters_length - 1) / 255 ];
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
  for (int row = 0; row < width; row++) {
    for (int col = 0; col < height; col++) {
      int pixelIndex = (row + y) * pixelWidth + (x + col);
      if (pixelIndex < pixelWidth * pixelHeight) {
        cv::Vec3b currentPixel = pixels[pixelIndex];
        value += get_grayscale(currentPixel[0], currentPixel[1], currentPixel[2]);
        valueCount++;
      }
    }
  }

  // std::cout << "Value: " << value << std::endl;
  // std::cout << "Value Count: " << valueCount << std::endl;

  if (valueCount > 0) {
    return value_characters[ value * (value_characters_length - 1) / (255 * valueCount) ];
  } else {
    return value_characters[0];
  }

}

void getWindowSize(int* width, int* height) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  *width = w.ws_col;
  *height = w.ws_row;
}

int get_grayscale(int r, int g, int b) {
  return (int)(0.299 * r + 0.587 * g + 0.114 * b);
}

void get_output_size(cv::Mat* target, int* width, int* height) {
  cv::Mat image = *target;

  if (image.cols <= windowWidth && image.rows <= windowHeight) {
    *width = image.cols;
    *height = image.rows;
  } else {

    double shrinkFactor = std::min((double)windowWidth / image.cols, (double)windowHeight / image.rows);

    *width = (int)(image.cols * shrinkFactor);
    *height = (int)(image.rows * shrinkFactor);
  }

}