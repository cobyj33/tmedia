#include <icons.h>
#include <image.h>
#include <ascii.h>
#include <ascii_constants.h>
#include <ascii_data.h>
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

std::map<int, pixel_data*> iconFromEnum;
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


pixel_data* emptyPixelData = (pixel_data*)malloc(sizeof(pixel_data));
bool initialized = false;

int testIconProgram() {
    init_icons();
    initscr();

    while (true) {
        for (int i = 0; i < 12; i++) {
            pixel_data* iconData = iconFromEnum.at(i);
            int outputWidth, outputHeight;
            get_output_size(iconData->width, iconData->height, COLS, LINES, &outputWidth, &outputHeight);

            ascii_image image = get_image(iconData->pixels, iconData->width, iconData->height, outputWidth, outputHeight);
            erase();
            print_ascii_image(&image);
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));    
        }
    }

    return EXIT_SUCCESS;
}

bool read_sprite_sheet(pixel_data** buffer, int bufferSize, pixel_data* iconData, int rows, int cols, int spriteWidth, int spriteHeight) {

    for (int i = 0; i < std::min(bufferSize, rows * cols); i++) {
        pixel_data* icon = pixel_data_alloc(spriteWidth, spriteHeight);
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
        pixel_data* playbackIconBuffer[numOfPlaybackIcons];
        pixel_data* numberIconBuffer[numOfNumberIcons];
        pixel_data* numberSymbolBuffer[numOfNumberSymbolIcons];

        pixel_data* playbackIcons = get_playback_icons_pixel_data();
        pixel_data* numberIcons = get_number_icons_pixel_data();
        pixel_data* numberSymbolIcons = get_number_symbols_icons_pixel_data();

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

pixel_data* get_video_icon(VideoIcon iconEnum) {
    if (iconFromEnum.count(iconEnum) == 1) {
        return iconFromEnum.at(iconEnum);
    } else {
        if (initialized) {
            std::cout << "ERROR: ICON ENUM " << iconEnum << " NOT INITIALIZED " << std::endl;
        } else {
            std::cout << "ICONS NOT INITIALIZED" << std::endl;
        }

        return emptyPixelData;
    }
}

VideoSymbol* get_video_symbol(VideoIcon iconEnum) {
    VideoSymbol* symbol = (VideoSymbol*)malloc(sizeof(VideoSymbol));
    symbol->startTime = std::chrono::steady_clock::now(); 
    symbol->lifeTime = std::chrono::milliseconds(1000);
    symbol->frames = 1;
    symbol->frameData = (pixel_data**)malloc(sizeof(pixel_data*) * symbol->frames);
    symbol->frameData[0] = get_video_icon(iconEnum);
    return symbol;
}

void free_video_symbol(VideoSymbol* symbol) {
    free(symbol->frameData);
    free(symbol);
}

VideoSymbol* get_symbol_from_volume(double normalizedVolume) {
    normalizedVolume = std::min(1.0, std::max(0.0, normalizedVolume));
    return get_video_symbol(volumeIcons[(int)(3.99 * normalizedVolume)]);
}



pixel_data* get_stitched_image(pixel_data* images) {
    pixel_data* pixelData = (pixel_data*)malloc(sizeof(pixel_data));
    int width;
    int height;
    

    return pixelData;
}



