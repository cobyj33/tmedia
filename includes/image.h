#pragma once
#include "pixeldata.h"
#include "color.h"

void get_output_size(int srcWidth, int srcHeight, int maxWidth, int maxHeight, int* width, int* height);
void get_scale_size(int srcWidth, int srcHeight, int targetWidth, int targetHeight, int* width, int* height);
void pixel_data_to_rgb(PixelData* data, rgb* output);
int imageProgram(const char* fileName, bool use_colors);

