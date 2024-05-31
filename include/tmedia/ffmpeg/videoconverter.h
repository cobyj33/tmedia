#ifndef TMEDIA_VIDEO_CONVERTER_H
#define TMEDIA_VIDEO_CONVERTER_H


#include <tmedia/util/defines.h>

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
     * Convert a video frame according to the initialized VideoConverter's
     * parameters and place the result into the AVFrame pointed to by dest.
     *
     * dest must have been allocated by av_frame_alloc already or a function
     * which calls av_frame_alloc. dest's buffers will be unreferenced by
     * av_frame_unref() whenever convert_video_frame is called. The source
     * AVFrame is left unaltered. The caller is responsible for freeing dest,
     * just as it was responsible for allocating dest.
     *
     * The src AVFrame is REQUIRED to have the same source width,
     * source height, and source pixel format as given
     * to the VideoConverter upon construction. Otherwise, convert_video_frame
     * will fail and throw an error.
     * 
     * After calling convert_video_frame and upon success, dest have the same
     * destination width, destination height,
     * and destination pixel format as given to the VideoConverter
     * upon construction
    */
    void convert_video_frame(AVFrame* dest, AVFrame* src);

    TMEDIA_ALWAYS_INLINE inline int get_src_width() { return this->m_src_width; }
    TMEDIA_ALWAYS_INLINE inline int get_src_height() { return this->m_src_height; }
    TMEDIA_ALWAYS_INLINE inline enum AVPixelFormat get_src_pix_fmt() { return this->m_src_pix_fmt; }
    TMEDIA_ALWAYS_INLINE inline int get_dst_width() { return this->m_dst_width; }
    TMEDIA_ALWAYS_INLINE inline int get_dst_height() { return this->m_dst_height; }
    TMEDIA_ALWAYS_INLINE inline enum AVPixelFormat get_dst_pix_fmt() { return this->m_dst_pix_fmt; }

    /**
     * Reset the destination frame size to be equal to a new size. Useful
     * for dynamically changing how large a frame should be rendered.
     *
     * No-op if the current dst_width and dst_height equal to the passed in
     * dst_width and dst_height
    */
    void reset_dst_size(int dst_width, int dst_height);

    ~VideoConverter();
};

#endif