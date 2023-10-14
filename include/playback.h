#ifndef ASCII_VIDEO_PLAYBACK_H
#define ASCII_VIDEO_PLAYBACK_H

#include "mediaplayer.h"

class MediaPlaybackContext {
  MediaPlayer media;
  MediaClock clock;
  MediaGUI guiSettings;

  MediaPlaybackContext(MediaPlayer media);
};

void asv_player_set_media();

void asv_player_remove_current_media();

#endif