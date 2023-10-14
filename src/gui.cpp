#include "gui.h"

#include <stdexcept>

void MediaGUI::set_video_output_mode(VideoOutputMode mode) {
  if (mode == this->m_video_output_mode)
    return;

  // if (mode == VideoOutputMode::COLORED || mode == VideoOutputMode::COLORED_BACKGROUND_ONLY) {
  //   ncurses_set_color_palette(AVNCursesColorPalette::RGB);
  // } else if (mode == VideoOutputMode::GRAYSCALE || mode == VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY) {
  //   ncurses_set_color_palette(AVNCursesColorPalette::GRAYSCALE);
  // }

  this->m_video_output_mode = mode;
}

MediaScreen MediaGUI::get_media_screen() const {
  return this->m_screen;
}

VideoOutputMode MediaGUI::get_video_output_mode() const {
  return this->m_video_output_mode;
}

VideoOutputMode get_video_output_mode_from_params(bool colors, bool grayscale, bool background) {
  if (background) {
    if (colors) {
      return VideoOutputMode::COLORED_BACKGROUND_ONLY;
    } else if (grayscale) {
      return VideoOutputMode::GRAYSCALE_BACKGROUND_ONLY;
    } else {
      throw std::runtime_error("Background mode must either be colored or grayscale");
    }
  }

  if (colors) {
    return VideoOutputMode::COLORED;
  } else if (grayscale) {
    return VideoOutputMode::GRAYSCALE;
  }

  return VideoOutputMode::TEXT_ONLY;
}
