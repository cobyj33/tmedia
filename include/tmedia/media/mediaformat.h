#ifndef TMEDIA_MEDIA_FORMAT_H
#define TMEDIA_MEDIA_FORMAT_H

/**
 * tmedia/media/mediaformat.h: externed string lists of the ffmpeg iformat names and extensions
 * recognized by tmedia to be recognized with av_match_list.
 * 
 * Note that many formats, such as mpeg4 and webm, aren't listed as any type
 * of iformat name or extension. This is because these file formats are moreso
 * media containers and can contain audio, video, or images. These strings
 * are specific for file formats that can only be a single type of media. With
 * containers, other probing must be done in order to correctly identify a 
 * media type.
*/

#include <tmedia/media/mediatype.h>

extern const char* banned_iformat_names;

extern const char* image_iformat_names;
extern const char* audio_iformat_names;
extern const char* video_iformat_names;

extern const char* image_iformat_exts;
extern const char* audio_iformat_exts;
extern const char* video_iformat_exts;

extern const char* container_iformat_exts;

// null terminated list of file extensions. Each file extension includes
// its period in its string in order to fufill equalness with 
// std::filesystem::path::extension().c_str()

struct mext_mdata {
  const char* ext;
  MediaType type;
  mext_mdata(const char* ext, MediaType type) : ext(ext), type(type) {} 
};

extern const mext_mdata mm_exts[];

#endif