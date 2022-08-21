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
pixel_data* emptyPixelData = (pixel_data*)malloc(sizeof(pixel_data));
bool initialized = false;


int testIconProgram() {
    init_icons();
    initscr();

    while (true) {
        for (int i = 0; i < 12; i++) {
            pixel_data* iconData = iconFromEnum.at(i);
            int windowWidth, windowHeight, outputWidth, outputHeight;
            get_window_size(&windowWidth, &windowHeight);
            get_output_size(iconData->width, iconData->height, windowWidth, windowHeight, &outputWidth, &outputHeight);

            ascii_image image = get_image(iconData->pixels, iconData->width, iconData->height, outputWidth, outputHeight);
            erase();
            print_ascii_image(&image);
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));    
        }
    }

    return EXIT_SUCCESS;
}

bool read_sprite_sheet(pixel_data** buffer, int bufferSize, const char* sheetPath, int rows, int cols, int spriteWidth, int spriteHeight) {
    pixel_data* iconData = get_pixel_data_from_image(sheetPath);

    if (iconData == nullptr) {
        std::cout << "Could not Initialize " << sheetPath << std::endl;
        return false;
    }

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

    pixel_data_free(iconData);
    return true;
}

bool init_icons() {
    if (!initialized) {

        pixel_data* buffer[numOfIcons];
        bool success = read_sprite_sheet(buffer, numOfIcons, "assets/video Icons.jpg", ICONS_SPRITE_ROWS, ICONS_SPRITE_COLUMNS, ICONS_SPRITE_WIDTH, ICONS_SPRITE_HEIGHT);

        if (!success) {
            std::cout << "Could not initialize icons" << std::endl;
            for (int i = 0; i < numOfIcons; i++) {
                if (buffer[i] != nullptr) {
                    pixel_data_free(buffer[i]);
                }
            }
            return false;
        }

        for (int i = 0; i < numOfIcons; i++) {
            iconFromEnum.emplace(std::make_pair(iconOrder[i], buffer[i]));
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

VideoSymbol get_video_symbol(VideoIcon iconEnum) {
    VideoSymbol symbol;
    symbol.framesToDelete = 10;
    symbol.framesShown = 0;
    symbol.pixelData = get_video_icon(iconEnum);
    return symbol;
}

VideoSymbol get_symbol_from_volume(double normalizedVolume) {
    normalizedVolume = std::min(1.0, std::max(0.0, normalizedVolume));
    return get_video_symbol(volumeIcons[(int)(3.99 * normalizedVolume)]);
}



