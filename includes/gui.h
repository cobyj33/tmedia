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
        VideoOutputMode m_video_output_mode = VideoOutputMode::TEXT_ONLY;
        MediaScreen m_screen = MediaScreen::VIDEO;

    public:
        GUIState() : m_video_output_mode(VideoOutputMode::TEXT_ONLY), m_screen(MediaScreen::VIDEO) {}
        GUIState(VideoOutputMode video_output_mode, MediaScreen screen) : m_video_output_mode(video_output_mode), m_screen(screen) {}

        VideoOutputMode get_video_output_mode() const;
        MediaScreen get_media_screen() const;
        void set_video_output_mode(VideoOutputMode mode);
};

VideoOutputMode get_video_output_mode_from_params(bool colors, bool grayscale, bool background);

#endif