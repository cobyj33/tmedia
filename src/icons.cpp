#include "pixeldata.h"
#include "renderer.h"
#include <icons.h>
#include <image.h>
#include <ascii.h>
#include <ncurses.h>


#include <thread>
#include <chrono>
#include <map>
#include <iostream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
    #include <libavdevice/avdevice.h>
    #include <libavutil/fifo.h>
    #include <libavutil/audio_fifo.h>
}

std::map<int, PixelData*> iconFromEnum;
std::map<int, VideoIcon> iconFromDigit = {
    { 0, ZERO_ICON },
    { 1, ONE_ICON },
    { 2, TWO_ICON },
    { 3, THREE_ICON },
    { 4, FOUR_ICON },
    { 5, FIVE_ICON },
    { 6, SIX_ICON },
    { 7, SEVEN_ICON },
    { 8, EIGHT_ICON },
    { 9, NINE_ICON }
};


bool initialized = false;

int testIconProgram() {
    for (int i = 0; i < 12; i++) {
        erase();
        PixelData* iconData = iconFromEnum.at(i);
        AsciiImage image = get_ascii_image_bounded(iconData, COLS, LINES);

        /* printw("Image %d: %d x %d to %d x %d\n", i, iconData->width, iconData->height, image.width, image.height); */
        /* for (int i = 0; i < image.height; i++) { */
        /*     for (int j = 0; j < image.width; j++) { */
        /*         addch(image.lines[i][j]); */
        /*     } */
        /*     addch('\n'); */
        /* } */
                
        print_ascii_image_full(&image);
        refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));    
    }

    return EXIT_SUCCESS;
}

bool read_sprite_sheet(PixelData** buffer, int bufferSize, PixelData* iconData, int rows, int cols, int spriteWidth, int spriteHeight) {

    for (int i = 0; i < std::min(bufferSize, rows * cols); i++) {
        PixelData* icon = pixel_data_alloc(spriteWidth, spriteHeight, GRAYSCALE8);
        int currentSpriteY = i / cols * spriteHeight;
        int currentSpriteX = i % cols * spriteWidth;
        for (int row = 0; row < spriteHeight; row++) {
            for (int col = 0; col < spriteWidth; col++) {
                icon->pixels[row * spriteWidth + col] = iconData->pixels[(currentSpriteY + row) * (spriteWidth * cols) + (currentSpriteX + col)];
            }
        }
        buffer[i] = icon;
    }

    return true;
}

bool init_icons() {
    if (!initialized) {
        bool success;
        PixelData* playbackIconBuffer[numOfPlaybackIcons];
        PixelData* numberIconBuffer[numOfNumberIcons];
        PixelData* numberSymbolBuffer[numOfNumberSymbolIcons];

        PixelData* playbackIcons = get_playback_icons_pixel_data();
        PixelData* numberIcons = get_number_icons_pixel_data();
        PixelData* numberSymbolIcons = get_number_symbols_icons_pixel_data();

        auto cleanup = [playbackIcons, numberIcons, numberSymbolIcons]() {
            free(playbackIcons);
            free(numberIcons);
            free(numberSymbolIcons);
        };

        success = read_sprite_sheet(playbackIconBuffer, numOfPlaybackIcons, playbackIcons, ICONS_SPRITE_ROWS, ICONS_SPRITE_COLUMNS, ICONS_SPRITE_WIDTH, ICONS_SPRITE_HEIGHT);
        if (!success) {
            std::cout << "Could not initialize playback icons" << std::endl;
            cleanup();
            return false;
        }

        for (int i = 0; i < numOfPlaybackIcons; i++) {
            iconFromEnum.emplace(std::make_pair(playbackIconOrder[i], playbackIconBuffer[i]));
        }

        success = read_sprite_sheet(numberIconBuffer, numOfNumberIcons, numberIcons, NUMBERS_SPRITE_ROWS, NUMBERS_SPRITE_COLUMNS, NUMBERS_SPRITE_WIDTH, NUMBERS_SPRITE_HEIGHT);
        if (!success) {
            std::cout << "Could not initialize number icons" << std::endl;
            cleanup();
            return false;
        }

        for (int i = 0; i < numOfNumberIcons; i++) {
            iconFromEnum.emplace(std::make_pair(numberIconOrder[i], numberIconBuffer[i]));
        }

        success = read_sprite_sheet(numberSymbolBuffer, numOfNumberSymbolIcons, numberSymbolIcons, NUMBER_SYMBOLS_SPRITE_ROWS, NUMBER_SYMBOLS_SPRITE_COLUMNS, NUMBER_SYMBOLS_SPRITE_WIDTH, NUMBER_SYMBOLS_SPRITE_HEIGHT);
        if (!success) {
            std::cout << "Could not initialize number symbol icons" << std::endl;
            cleanup();
            return false;
        }

        for (int i = 0; i < numOfNumberSymbolIcons; i++) {
            iconFromEnum.emplace(std::make_pair(numberSymbolIconOrder[i], numberSymbolBuffer[i]));
        }

        initialized = true;
    }

    std::cout << "All Icons Initialized" << std::endl;
    return initialized;
}

bool free_icons() {
    if (initialized) {
        for (auto itr = iconFromEnum.begin(); itr != iconFromEnum.end(); itr++) {
            pixel_data_free(itr->second);
        }
        iconFromEnum.clear();
        initialized = false;
    }
    return !initialized;
}

PixelData* get_video_icon(VideoIcon iconEnum) {
    if (iconFromEnum.count(iconEnum) == 1) {
        return iconFromEnum.at(iconEnum);
    } else {
        if (initialized) {
            std::cout << "ERROR: ICON ENUM " << iconEnum << " NOT INITIALIZED " << std::endl;
        } else {
            std::cout << "ICONS NOT INITIALIZED" << std::endl;
        }

        return nullptr;
    }
}

VideoSymbol* get_video_symbol(VideoIcon iconEnum) {
    VideoSymbol* symbol = (VideoSymbol*)malloc(sizeof(VideoSymbol));
    symbol->startTime = std::chrono::steady_clock::now(); 
    symbol->lifeTime = std::chrono::milliseconds(1000);
    symbol->frames = 1;
    symbol->frameData = (PixelData**)malloc(sizeof(PixelData*) * symbol->frames);
    symbol->frameData[0] = get_video_icon(iconEnum);
    return symbol;
}

VideoSymbol* copy_video_symbol(VideoSymbol* original) {
    VideoSymbol* copiedSymbol = (VideoSymbol*)malloc(sizeof(VideoSymbol));
    copiedSymbol->startTime = original->startTime;
    copiedSymbol->lifeTime = original->lifeTime;
    copiedSymbol->frames = original->frames;
    copiedSymbol->frameData = (PixelData**)malloc(sizeof(PixelData*) * original->frames);
    
    for (int i = 0; i < original->frames; i++) {
        copiedSymbol->frameData[i] = original->frameData[i];
    }
    return copiedSymbol;
}

void free_video_symbol(VideoSymbol* symbol) {
    free(symbol->frameData);
    free(symbol);
}

VideoSymbol* get_symbol_from_volume(double normalizedVolume) {
    normalizedVolume = std::min(1.0, std::max(0.0, normalizedVolume));
    return get_video_symbol(volumeIcons[(int)(3.99 * normalizedVolume)]);
}


int get_video_symbol_current_frame(VideoSymbol* symbol) {
    return  (int)((double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - symbol->startTime).count() / (double)symbol->lifeTime.count() * (double)symbol->frames); 
}


/* PixelData* get_stitched_image(PixelData** images) { */
/*     PixelData* pixelData = (PixelData*)malloc(sizeof(PixelData)); */
/*     int width; */
/*     int height; */
    

/*     return pixelData; */
/* } */



