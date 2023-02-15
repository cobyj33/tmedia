#ifndef ASCII_VIDEO_ICONS
#define ASCII_VIDEO_ICONS

#include "image.h"
#include "pixeldata.h"
#include <string>
#include <ctime>


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

// enum class VideoIconType {
//     PLAYBACK,
//     VOLUME,
//     DIGIT
// };

class VideoIcon {
    public:
        const PixelData pixelData;

        static const VideoIcon STOP_ICON;
        static const VideoIcon PLAY_ICON;
        static const VideoIcon PAUSE_ICON;
        static const VideoIcon FORWARD_ICON;
        static const VideoIcon BACKWARD_ICON;
        static const VideoIcon MUTE_ICON;
        static const VideoIcon NO_VOLUME_ICON;
        static const VideoIcon LOW_VOLUME_ICON;
        static const VideoIcon MEDIUM_VOLUME_ICON;
        static const VideoIcon HIGH_VOLUME_ICON;
        static const VideoIcon MAXIMIZED_ICON;
        static const VideoIcon MINIMIZED_ICON;
        static const VideoIcon ZERO_ICON;
        static const VideoIcon ONE_ICON;
        static const VideoIcon TWO_ICON;
        static const VideoIcon THREE_ICON;
        static const VideoIcon FOUR_ICON;
        static const VideoIcon FIVE_ICON;
        static const VideoIcon SIX_ICON;
        static const VideoIcon SEVEN_ICON;
        static const VideoIcon EIGHT_ICON;
        static const VideoIcon NINE_ICON;
        static const VideoIcon POINT_ICON;
        static const VideoIcon TIMES_ICON;
        static const VideoIcon DIVIDE_ICON;
        static const VideoIcon PLUS_ICON;
        static const VideoIcon MINUS_ICON;

        static const VideoIcon ERROR_ICON;

        static const VideoIcon get_digit_icon(int digit);
        static const VideoIcon get_volume_icon(double percentage);

        VideoIcon(PixelData pixelData) : pixelData(pixelData) {}
};

#endif
