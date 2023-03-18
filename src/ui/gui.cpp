#include "gui.h"

#include <termcolor.h>


void GUIState::set_video_output_mode(VideoOutputMode mode) {
    switch (mode) {
        case VideoOutputMode::COLORED: ncurses_initialize_color_palette();

    }
    this->m_video_output_mode = mode;
}

MediaScreen GUIState::get_media_screen() const {
    return this->m_screen;
}


VideoOutputMode GUIState::get_video_output_mode() const {
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
