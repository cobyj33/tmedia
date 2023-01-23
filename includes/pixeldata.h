#ifndef ASCII_VIDEO_PIXEL_DATA
#define ASCII_VIDEO_PIXEL_DATA

extern "C" {
#include <stdint.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

typedef enum PixelDataFormat {
    RGB24, GRAYSCALE8
} PixelDataFormat;

typedef struct PixelData {
    uint8_t* pixels;
    int width;
    int height;
    PixelDataFormat format;
} PixelData;

enum AVPixelFormat PixelDataFormat_to_AVPixelFormat(PixelDataFormat format);
PixelDataFormat AVPixelFormat_to_PixelDataFormat(enum AVPixelFormat format);
PixelData* copy_pixel_data(PixelData* original);
PixelData* pixel_data_alloc(int width, int height, PixelDataFormat);
PixelData* pixel_data_alloc_from_frame(AVFrame* videoFrame);
int get_pixel_data_buffer_size(PixelData* data);
void pixel_data_free(PixelData* PixelData);
PixelData* get_pixel_data_from_image(const char* fileName, PixelDataFormat format);
const char* pixel_data_format_string(PixelDataFormat format);
int pixel_data_equals(PixelData* first, PixelData* second);
#endif
