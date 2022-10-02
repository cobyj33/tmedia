#ifndef ASCII_VIDEO_RENDERER
#define ASCII_VIDEO_RENDERER
#include <ascii.h>
#include <media.h>

typedef struct AudioGuiData {
    int channel_index;
    int show_all_channels;
} AudioGuiData;

typedef struct VideoGuiData {
    int show_color;
    int fullscreen;
} VideoGuiData;

typedef struct GuiData {
    MediaDisplayMode mode;
    AudioGuiData audio;
    VideoGuiData video;
    int show_debug;
} GuiData;

void render_screen(MediaPlayer* player, GuiData gui_data);
void render_movie_screen(MediaPlayer* player, GuiData gui_data);
void render_video_debug(MediaPlayer* player, GuiData gui_data);
void render_audio_debug(MediaPlayer* player, GuiData gui_data);
void render_audio_screen(MediaPlayer* player, GuiData gui_data);

void print_ascii_image_full(AsciiImage* textImage);
#endif
