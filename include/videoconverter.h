#ifndef ASCII_VIDEO_VIDEO_CONVERTER_INCLUDE
#define ASCII_VIDEO_VIDEO_CONVERTER_INCLUDE

extern "C" {
  #include <libavutil/frame.h>
  #include <libswscale/swscale.h>
}

/**
 * @brief Basic wrapper around FFmpeg's SwsContext, which provides a context for FFmpeg for image rescaling and pixel format conversions
 */
class VideoConverter {
  private:
    struct SwsContext* m_context;
    int m_src_width;
    int m_src_height;
    enum AVPixelFormat m_src_pix_fmt;
    int m_dst_width;
    int m_dst_height;
    enum AVPixelFormat m_dst_pix_fmt;
  public:
    VideoConverter(int dst_width,
            int dst_height,
            enum AVPixelFormat dst_pix_fmt,
            int src_width,
            int src_height,
            enum AVPixelFormat src_pix_fmt);

    AVFrame* convert_video_frame(AVFrame* original);

    int get_src_width();
    int get_src_height();
    enum AVPixelFormat get_src_pix_fmt();
    int get_dst_width();
    int get_dst_height();
    enum AVPixelFormat get_dst_pix_fmt();

    void reset_dst_size(int dst_width, int dst_height);

    ~VideoConverter();
};

#endif