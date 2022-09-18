#pragma once
#include "image.h"

#include <chrono>

#define ICONS_SPRITE_WIDTH 16
#define ICONS_SPRITE_HEIGHT 16
#define ICONS_SPRITE_ROWS 3
#define ICONS_SPRITE_COLUMNS 5

#define NUMBERS_SPRITE_WIDTH 5
#define NUMBERS_SPRITE_HEIGHT 14
#define NUMBERS_SPRITE_ROWS 1
#define NUMBERS_SPRITE_COLUMNS 10

#define NUMBER_SIG_FIGS 3

#define NUMBER_SYMBOLS_SPRITE_WIDTH 5
#define NUMBER_SYMBOLS_SPRITE_HEIGHT 5
#define NUMBER_SYMBOLS_SPRITE_ROWS 1
#define NUMBER_SYMBOLS_SPRITE_COLUMNS 5

#define PLAYBACK_ICONS_PATH "assets/playback Icons.jpg"
#define NUMBER_ICONS_PATH "assets/video Numbers.jpg"
#define NUMBER_SYMBOLS_ICONS_PATH "assets/video Number Symbols.jpg"

#define VIDEO_SYMBOL_BUFFER_SIZE 10

typedef enum VideoIcon {
    STOP_ICON, PLAY_ICON, PAUSE_ICON, FORWARD_ICON, BACKWARD_ICON, MUTE_ICON, NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON, MAXIMIZED_ICON, MINIMIZED_ICON, ZERO_ICON, ONE_ICON, TWO_ICON, THREE_ICON, FOUR_ICON, FIVE_ICON, SIX_ICON, SEVEN_ICON, EIGHT_ICON, NINE_ICON, POINT_ICON, TIMES_ICON, DIVIDE_ICON, PLUS_ICON, MINUS_ICON
} VideoIcon;

typedef struct VideoSymbol {
    std::chrono::steady_clock::time_point startTime;
    std::chrono::milliseconds lifeTime;
    int frames;
    PixelData** frameData;
} VideoSymbol;

const int numOfPlaybackIcons = 12;
const VideoIcon playbackIconOrder[numOfPlaybackIcons] = { 
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

const int numOfNumberIcons = 10;
const VideoIcon numberIconOrder[numOfNumberIcons] = {
   ONE_ICON,
   TWO_ICON,
   THREE_ICON,
   FOUR_ICON,
   FIVE_ICON,
   SIX_ICON,
   SEVEN_ICON,
   EIGHT_ICON,
   NINE_ICON
};


const int numOfNumberSymbolIcons = 5;
const VideoIcon numberSymbolIconOrder[numOfNumberSymbolIcons] = {
    POINT_ICON,
    TIMES_ICON,
    PLUS_ICON,
    DIVIDE_ICON,
    MINUS_ICON
};


const VideoIcon volumeIcons[4] = { NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON };

int testIconProgram();
bool init_icons();
bool free_icons();
bool read_sprite_sheet(PixelData** buffer, int bufferSize, const char* sheetPath, int rows, int cols, int spriteWidth, int spriteHeight);
PixelData* get_video_icon(VideoIcon iconEnum);
VideoSymbol* get_video_symbol(VideoIcon iconEnum);
VideoSymbol* get_symbol_from_volume(double normalizedVolume);
VideoSymbol* copy_video_symbol(VideoSymbol* original);
int get_video_symbol_current_frame(VideoSymbol* symbol);
void free_video_symbol(VideoSymbol* symbol);

typedef struct VideoSymbolStack {
    VideoSymbol* symbols[VIDEO_SYMBOL_BUFFER_SIZE];
    int top;
} VideoSymbolStack;

VideoSymbolStack* video_symbol_stack_alloc();
void video_symbol_stack_free(VideoSymbolStack* stack);
void video_symbol_stack_push(VideoSymbolStack* stack, VideoSymbol* symbol);
VideoSymbol* video_symbol_stack_pop(VideoSymbolStack* stack);
void video_symbol_stack_erase_pop(VideoSymbolStack* stack);
VideoSymbol* video_symbol_stack_peek(VideoSymbolStack* stack);
void video_symbol_stack_clear(VideoSymbolStack* stack);

PixelData* get_playback_icons_pixel_data();
PixelData* get_number_icons_pixel_data(); 
PixelData* get_number_symbols_icons_pixel_data(); 
PixelData* get_numbers_image(int* digits, int count);
