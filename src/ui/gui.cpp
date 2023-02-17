#include "gui.h"

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
