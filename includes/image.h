#pragma once
#include <cstdint>

extern "C" {
#include <libavutil/frame.h>
}

typedef struct pixel_data {
    uint8_t* pixels;
    int width;
    int height;
} pixel_data;

pixel_data* copy_pixel_data(pixel_data* original);
pixel_data* pixel_data_alloc(int width, int height);
pixel_data* pixel_data_alloc_from_frame(AVFrame* videoFrame);
void pixel_data_free(pixel_data* pixel_data);
pixel_data* get_pixel_data_from_image(const char* fileName);
void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height);
void get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight, int* width, int* height);

int imageProgram(const char* fileName);
