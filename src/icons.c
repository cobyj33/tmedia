#include "macros.h"
#include "pixeldata.h"
#include "renderer.h"
#include "threads.h"
#include <icons.h>
#include <image.h>
#include <ascii.h>
#include <ncurses.h>

#include <pthread.h>
#include <wtime.h>
#include <wmath.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include <libavutil/fifo.h>
#include <libavutil/audio_fifo.h>

#define NUMBER_OF_VIDEO_ICONS 27
#define NUMBER_OF_PLAYBACK_ICONS 12
#define NUMBER_OF_NUMBER_ICONS 10
#define NUMBER_OF_NUMBER_SYMBOL_ICONS 5

#define NUMBER_OF_VOLUME_ICONS 4

const VideoIcon videoIcons[NUMBER_OF_VIDEO_ICONS] = { STOP_ICON, PLAY_ICON, PAUSE_ICON, FORWARD_ICON, BACKWARD_ICON, MUTE_ICON, NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON, MAXIMIZED_ICON, MINIMIZED_ICON, ZERO_ICON, ONE_ICON, TWO_ICON, THREE_ICON, FOUR_ICON, FIVE_ICON, SIX_ICON, SEVEN_ICON, EIGHT_ICON, NINE_ICON, POINT_ICON, TIMES_ICON, DIVIDE_ICON, PLUS_ICON, MINUS_ICON };

const VideoIcon playbackIconOrder[NUMBER_OF_PLAYBACK_ICONS] = { 
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

const VideoIcon numberIconOrder[NUMBER_OF_NUMBER_ICONS] = {
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

const VideoIcon numberSymbolIconOrder[NUMBER_OF_NUMBER_SYMBOL_ICONS] = {
    POINT_ICON,
    TIMES_ICON,
    PLUS_ICON,
    DIVIDE_ICON,
    MINUS_ICON
};

const VideoIcon volumeIcons[NUMBER_OF_VOLUME_ICONS] = { NO_VOLUME_ICON, LOW_VOLUME_ICON, MEDIUM_VOLUME_ICON, HIGH_VOLUME_ICON };

PixelData* icons[NUMBER_OF_VIDEO_ICONS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


VideoIcon iconFromDigit(int digit) {
    switch (digit % 10) {
        case 0: return ZERO_ICON;
        case 1: return ONE_ICON;
        case 2: return TWO_ICON;
        case 3: return THREE_ICON;
        case 4: return FOUR_ICON;
        case 5: return FIVE_ICON;
        case 6: return SIX_ICON;
        case 7: return SEVEN_ICON;
        case 8: return EIGHT_ICON;
        case 9: return NINE_ICON;
        default: return ZERO_ICON;
    }
}


int initialized = 0;
int testIconProgram() {
    for (int i = 0; i < 12; i++) {
        erase();
        PixelData* iconData = icons[i];
        AsciiImage* image = get_ascii_image_bounded(iconData, COLS, LINES);
        if (image != NULL) {
            print_ascii_image_full(image);
            refresh();
            ascii_image_free(image);
        }
                
        sleep_for_ms(500);
    }

    return EXIT_SUCCESS;
}

int read_sprite_sheet(PixelData** buffer, int bufferSize, PixelData* iconData, int rows, int cols, int spriteWidth, int spriteHeight) {

    for (int i = 0; i < i32min(bufferSize, rows * cols); i++) {
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

    return 1;
}

int init_icons() {
    if (!initialized) {
        bool success;
        PixelData* playbackIconBuffer[NUMBER_OF_PLAYBACK_ICONS];
        PixelData* numberIconBuffer[NUMBER_OF_NUMBER_ICONS];
        PixelData* numberSymbolBuffer[NUMBER_OF_NUMBER_SYMBOL_ICONS];

        PixelData* playbackIcons = get_playback_icons_pixel_data();
        PixelData* numberIcons = get_number_icons_pixel_data();
        PixelData* numberSymbolIcons = get_number_symbols_icons_pixel_data();

        success = read_sprite_sheet(playbackIconBuffer, NUMBER_OF_PLAYBACK_ICONS, playbackIcons, ICONS_SPRITE_ROWS, ICONS_SPRITE_COLUMNS, ICONS_SPRITE_WIDTH, ICONS_SPRITE_HEIGHT);
        if (!success) {
            fprintf(stderr, "%s\n", "Could Not initialize playback icons");
            pixel_data_free(playbackIcons);
            pixel_data_free(numberIcons);
            pixel_data_free(numberSymbolIcons);
            return 0;
        }

        for (int i = 0; i < NUMBER_OF_PLAYBACK_ICONS; i++) {
            icons[playbackIconOrder[i]] = playbackIconBuffer[i];
        }

        success = read_sprite_sheet(numberIconBuffer, NUMBER_OF_NUMBER_ICONS, numberIcons, NUMBERS_SPRITE_ROWS, NUMBERS_SPRITE_COLUMNS, NUMBERS_SPRITE_WIDTH, NUMBERS_SPRITE_HEIGHT);
        if (!success) {
            fprintf(stderr, "%s\n", "Could Not initialize number icons");
            pixel_data_free(playbackIcons);
            pixel_data_free(numberIcons);
            pixel_data_free(numberSymbolIcons);
            return 0;
        }

        for (int i = 0; i < NUMBER_OF_NUMBER_ICONS; i++) {
            icons[numberIconOrder[i]] = numberIconBuffer[i];
        }

        success = read_sprite_sheet(numberSymbolBuffer, NUMBER_OF_NUMBER_SYMBOL_ICONS, numberSymbolIcons, NUMBER_SYMBOLS_SPRITE_ROWS, NUMBER_SYMBOLS_SPRITE_COLUMNS, NUMBER_SYMBOLS_SPRITE_WIDTH, NUMBER_SYMBOLS_SPRITE_HEIGHT);
        if (!success) {
            fprintf(stderr, "%s\n", "Could Not initialize number symbol icons");
            pixel_data_free(playbackIcons);
            pixel_data_free(numberIcons);
            pixel_data_free(numberSymbolIcons);
            return 0;
        }

        for (int i = 0; i < NUMBER_OF_NUMBER_SYMBOL_ICONS; i++) {
            icons[numberSymbolIconOrder[i]] = numberSymbolBuffer[i];
        }

        initialized = 1;
    }

    printf("%s\n", "All Icons Initialized");
    return initialized;
}

int free_icons() {
    if (initialized) {
        for (int i = 0; i < NUMBER_OF_VIDEO_ICONS; i++) {
            if (icons[i] != NULL) {
                pixel_data_free(icons[i]);
            }
        }
        initialized = 0;
    }
    return !initialized;
}

PixelData* get_video_icon(VideoIcon iconEnum) {
    if (icons[iconEnum] != NULL) {
        return icons[iconEnum];
    } else {
        if (initialized) {
            fprintf(stderr, "%s %d %s\n", "ERROR: ICON ENUM ", iconEnum, " NOT INITIALIZED ");
        } else {
            fprintf(stderr, "%s\n", "ICONS NOT INITIALIZED");
        }

        return NULL;
    }
}

VideoSymbol* get_video_symbol(VideoIcon iconEnum) {
    VideoSymbol* symbol = (VideoSymbol*)malloc(sizeof(VideoSymbol));
    symbol->startTime = clock_sec(); 
    symbol->lifeTime = 1;
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
    normalizedVolume = fmin(1.0, fmax(0.0, normalizedVolume));
    return get_video_symbol(volumeIcons[(int)((NUMBER_OF_VOLUME_ICONS - 0.01) * normalizedVolume)]);
}


int get_video_symbol_current_frame(VideoSymbol* symbol) {
    return  ((clock_sec() - symbol->startTime) / symbol->lifeTime) * (double)symbol->frames; 
}


/* PixelData* get_stitched_image(PixelData** images) { */
/*     PixelData* pixelData = (PixelData*)malloc(sizeof(PixelData)); */
/*     int width; */
/*     int height; */
    

/*     return pixelData; */
/* } */



