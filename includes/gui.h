#ifndef ASCII_VIDEO_GUI_STATE
#define ASCII_VIDEO_GUI_STATE

#include <stdexcept>

enum class VideoOutputMode {
    COLORED,
    GRAYSCALE,
    COLORED_BACKGROUND_ONLY,
    GRAYSCALE_BACKGROUND_ONLY,
    TEXT_ONLY,
};

enum class MediaScreen {
    VIDEO,
    AUDIO
};

class GUIState {
    private:
        VideoOutputMode m_video_output_mode;
        MediaScreen m_screen;

    public:
        GUIState() {
            this->m_video_output_mode = VideoOutputMode::TEXT_ONLY;
            this->m_screen = MediaScreen::VIDEO;
        }

        VideoOutputMode get_video_output_mode() {
            return this->m_video_output_mode;
        }

        void set_video_output_mode(VideoOutputMode mode) {
            this->m_video_output_mode = mode;
        }
};

VideoOutputMode get_video_output_mode_from_params(bool colors, bool grayscale, bool background);

#endif