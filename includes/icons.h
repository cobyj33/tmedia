#pragma once
#include <ascii_data.h>

#define ICONS_SPRITE_WIDTH 16
#define ICONS_SPRITE_HEIGHT 16
#define ICONS_SPRITE_ROWS 3
#define ICONS_SPRITE_COLUMNS 5

#define NUMBERS_SPRITE_WIDTH 5
#define NUMEBRS_SPRITE_HEIGHT 14
#define NUMBERS_SPRITE_ROWS 1
#define NUMBERS_SPRITE_COLUMNS 10

#define NUMBER_SYMBOLS_SPRITE_WIDTH 5
#define NUMBER_SYMBOLS_SPRITE_HEIGHT 5
#define NUMBER_SYMBOLS_SPRITE_ROWS 1
#define NUMBER_SYMBOLS_SPRITE_COLUMNS 5

typedef enum VideoIcon {
    STOP_ICON, PLAY_ICON, PAUSE_ICON, FORWARD_ICON, BACKWARD_ICON, MUTE_ICON, NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON, MAXIMIZED_ICON, MINIMIZED_ICON,
} VideoIcon;

typedef struct VideoSymbol {
    int framesToDelete;
    int framesShown;
    pixel_data* pixelData;
} VideoSymbol;

const int numOfIcons = 12;
const VideoIcon iconOrder[numOfIcons] = { 
    STOP_ICON,
    PLAY_ICON,
    FORWARD_ICON,
    BACKWARD_ICON,
    PAUSE_ICON,
    NO_VOLUME_ICON,
    LOW_VOLUME_ICON,
    MEDIUM_VOLUME_ICON,
    HIGH_VOLUME_ICON,
    MUTE_ICON,
    MAXIMIZED_ICON,
    MINIMIZED_ICON,
};


const VideoIcon volumeIcons[4] = { NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON };

int testIconProgram();
bool init_icons();
bool free_icons();
bool read_sprite_sheet(pixel_data** buffer, int bufferSize, const char* sheetPath, int rows, int cols, int spriteWidth, int spriteHeight);
pixel_data* get_video_icon(VideoIcon iconEnum);
VideoSymbol get_video_symbol(VideoIcon iconEnum);
VideoSymbol get_symbol_from_volume(double normalizedVolume);
VideoSymbol get_symbol_from_playback_speed(double playbackSpeed);
