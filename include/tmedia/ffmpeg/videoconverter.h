#ifndef TMEDIA_VIDEO_CONVERTER_H
#define TMEDIA_VIDEO_CONVERTER_H

extern "C" {
  #include <libavutil/frame.h>
  #include <libswscale/swscale.h>
}

/**
 * Basic RAII wrapper around FFmpeg's SwsContext, which provides a context
 * for FFmpeg for image rescaling and pixel format conversions
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

    /**
     * 
     * The returned video frame must be freed by the caller with av_frame_free.
    */
    AVFrame* convert_video_frame(AVFrame* original);

    inline int get_src_width() { return this->m_src_width; }
    inline int get_src_height() { return this->m_src_height; }
    inline enum AVPixelFormat get_src_pix_fmt() { return this->m_src_pix_fmt; }
    inline int get_dst_width() { return this->m_dst_width; }
    inline int get_dst_height() { return this->m_dst_height; }
    inline enum AVPixelFormat get_dst_pix_fmt() { return this->m_dst_pix_fmt; }

    /**
     * No-op if the current dst_width and dst_height equal to the passed in
     * dst_width and dst_height
    */
    void reset_dst_size(int dst_width, int dst_height);

    ~VideoConverter();
};

#endif