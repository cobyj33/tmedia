#pragma once
#include <ascii.h>
#include <media.h>
void render_screen(MediaPlayer* player);
void render_movie_screen(MediaPlayer* player);
void render_video_debug(MediaPlayer* player);
void render_audio_debug(MediaPlayer* player);
void render_audio_screen(MediaPlayer* player);

void print_ascii_image_full(AsciiImage* textImage);
