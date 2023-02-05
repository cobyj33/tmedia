/**
 * @file icons.cpp
 * @author your name (you@domain.com)
 * @brief Hardcoded Icon Image Data into Ascii_Video
 * @version 0.1
 * @date 2023-01-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "pixeldata.h"
#include <icons.h>
#include <stdexcept>
#include <wmath.h>

// #define NUMBER_OF_VIDEO_ICONS 27
// #define NUMBER_OF_PLAYBACK_ICONS 12
// #define NUMBER_OF_NUMBER_ICONS 10
// #define NUMBER_OF_NUMBER_SYMBOL_ICONS 5

// #define NUMBER_OF_VOLUME_ICONS 4

std::vector<std::vector<uint8_t>> testMatrix{ std::vector<uint8_t>{0, 0, 0} };
PixelData test = PixelData(testMatrix);


std::vector<std::vector<uint8_t>> STOP_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::STOP_ICON = VideoIcon(PixelData(STOP_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> PLAY_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::PLAY_ICON = VideoIcon(PixelData(PLAY_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> PAUSE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::PAUSE_ICON = VideoIcon(PixelData(PAUSE_ICON_PIXEL_DATA));


std::vector<std::vector<uint8_t>> FORWARD_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 255, 255, 255 },
    {255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255 },
    {255, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255 },
    {255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::FORWARD_ICON = VideoIcon(PixelData(FORWARD_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> BACKWARD_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255, 0, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::BACKWARD_ICON = VideoIcon(PixelData(BACKWARD_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> MUTE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 0, 255, 255, 255, 0, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 255, 0, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 255, 0, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 255, 0, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 0, 255, 0, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 0, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::MUTE_ICON = VideoIcon(PixelData(MUTE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> NO_VOLUME_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::NO_VOLUME_ICON = VideoIcon(PixelData(NO_VOLUME_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> LOW_VOLUME_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::LOW_VOLUME_ICON = VideoIcon(PixelData(LOW_VOLUME_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> MEDIUM_VOLUME_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 0, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 255, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 0, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 0, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::MEDIUM_VOLUME_ICON = VideoIcon(PixelData(MEDIUM_VOLUME_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> HIGH_VOLUME_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 0, 255, 0, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 255, 0, 255, 0, 255 },
    {255, 255, 255, 255, 0, 0, 0, 0, 0, 255, 255, 255, 0, 255, 0, 255 },
    {255, 255, 255, 255, 255, 0, 0, 0, 0, 255, 255, 255, 0, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::HIGH_VOLUME_ICON = VideoIcon(PixelData(HIGH_VOLUME_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> MAXIMIZED_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255 },
    {255, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::MAXIMIZED_ICON = VideoIcon(PixelData(MAXIMIZED_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> MINIMIZED_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255 },
    {255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 0, 255, 255, 255, 255, 0, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::MINIMIZED_ICON = VideoIcon(PixelData(MINIMIZED_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> ZERO_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::ZERO_ICON = VideoIcon(PixelData(ZERO_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> ONE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::ONE_ICON = VideoIcon(PixelData(ONE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> TWO_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::TWO_ICON = VideoIcon(PixelData(TWO_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> THREE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::THREE_ICON = VideoIcon(PixelData(THREE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> FOUR_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::FOUR_ICON = VideoIcon(PixelData(FOUR_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> FIVE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::FIVE_ICON = VideoIcon(PixelData(FIVE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> SIX_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::SIX_ICON = VideoIcon(PixelData(SIX_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> SEVEN_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::SEVEN_ICON = VideoIcon(PixelData(SEVEN_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> EIGHT_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::EIGHT_ICON = VideoIcon(PixelData(EIGHT_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> NINE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 255, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::NINE_ICON = VideoIcon(PixelData(NINE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> POINT_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::POINT_ICON = VideoIcon(PixelData(POINT_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> TIMES_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 0, 255, 0, 255 },
    {255, 255, 0, 255, 255 },
    {255, 0, 255, 0, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::TIMES_ICON = VideoIcon(PixelData(TIMES_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> DIVIDE_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 255, 255, 0, 255 },
    {255, 255, 0, 255, 255 },
    {255, 0, 255, 255, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::DIVIDE_ICON = VideoIcon(PixelData(DIVIDE_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> PLUS_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 255, 0, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 0, 255, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::PLUS_ICON = VideoIcon(PixelData(PLUS_ICON_PIXEL_DATA));

std::vector<std::vector<uint8_t>> MINUS_ICON_PIXEL_DATA = std::vector<std::vector<uint8_t>>{
    {255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255 },
    {255, 0, 0, 0, 255 },
    {255, 255, 255, 255, 255 },
    {255, 255, 255, 255, 255 }
};
const VideoIcon VideoIcon::MINUS_ICON = VideoIcon(PixelData(MINUS_ICON_PIXEL_DATA));

const VideoIcon VideoIcon::get_digit_icon(int digit) {
    switch (digit % 10) {
        case 0: return VideoIcon::ZERO_ICON;
        case 1: return VideoIcon::ONE_ICON;
        case 2: return VideoIcon::TWO_ICON;
        case 3: return VideoIcon::THREE_ICON;
        case 4: return VideoIcon::FOUR_ICON;
        case 5: return VideoIcon::FIVE_ICON;
        case 6: return VideoIcon::SIX_ICON;
        case 7: return VideoIcon::SEVEN_ICON;
        case 8: return VideoIcon::EIGHT_ICON;
        case 9: return VideoIcon::NINE_ICON;
    }
    throw std::runtime_error("Cannot get icon from \"digit\" " + std::to_string(digit) + ". passed in digit value must be a digit (0, 1, 2, 3, 4, 5, 6, 7, 8, or 9)");
}

const VideoIcon VideoIcon::get_volume_icon(double percentage) {
    double normalizedVolume = clamp(percentage, 0.0, 1.0);
    
    if (percentage <= 0.0) {
        return VideoIcon::NO_VOLUME_ICON;
    } else if (percentage < 0.40) {
        return VideoIcon::LOW_VOLUME_ICON;
    } else if (percentage < 0.80) {
        return VideoIcon::MEDIUM_VOLUME_ICON;
    } else {
        return VideoIcon::HIGH_VOLUME_ICON;
    }
}