#include <video.h>
#include <image.h>
#include <ascii.h>
#include <ascii_constants.h>
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
const int SPRITE_WIDTH = 16;
const int SPRITE_HEIGHT = 16;
const int SPRITE_ROWS = 3;
const int SPRITE_COLUMNS = 5;

int numOfIcons = 12;
VideoIcon iconOrder[12] = { 
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
    MINIMIZED_ICON
};

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

bool init_icons() {
    if (!initialized) {
        bool success;
        pixel_data iconData = get_pixel_data_from_image("assets/video Icons.jpg", MAX_FRAME_WIDTH, MAX_FRAME_HEIGHT, &success);
        std::cout << "icon data retrieved" << std::endl;

        if (!success) {
            std::cout << "Could not Initialize video icons" << std::endl;
            return false;
        }

        for (int i = 0; i < numOfIcons; i++) {
            pixel_data* icon = pixel_data_alloc(SPRITE_WIDTH, SPRITE_HEIGHT);
            int currentSpriteY = i / SPRITE_COLUMNS * SPRITE_HEIGHT;
            int currentSpriteX = i % SPRITE_COLUMNS * SPRITE_WIDTH;
            std::cout << "Y: " << currentSpriteY << "X: " << currentSpriteX << std::endl;
            for (int row = 0; row < SPRITE_HEIGHT; row++) {
                for (int col = 0; col < SPRITE_WIDTH; col++) {
                    icon->pixels[row * SPRITE_WIDTH + col] = iconData.pixels[(currentSpriteY + row) * (SPRITE_WIDTH * SPRITE_COLUMNS) + (currentSpriteX + col)];
                }
            }
            std::cout << "icon " << i << " retrieved" << std::endl;

            iconFromEnum.emplace(std::make_pair(iconOrder[i], icon));
        }

        free(iconData.pixels);

        std::cout << "Icon Data Freed" << std::endl;

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

VideoIcon volumeIcons[4] = { NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON };
VideoSymbol get_symbol_from_volume(double normalizedVolume) {
    normalizedVolume = std::min(1.0, std::max(0.0, normalizedVolume));
    return get_video_symbol(volumeIcons[(int)(3.99 * normalizedVolume)]);
}



