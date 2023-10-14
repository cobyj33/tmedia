#ifndef ASCII_VIDEO_RENDERER_H
#define ASCII_VIDEO_RENDERER_H

#include "mediaplayer.h"

void ascii_video_set_current_media(MediaPlayer* media);

bool ascii_video_has_media_player();
void ascii_video_remove_current_media();

#endif